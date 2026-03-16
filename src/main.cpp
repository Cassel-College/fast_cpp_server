#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "ArgumentParser.h"
#include "FreeFunc.h"
#include "InitTools.h"
#include "MyDoctor.h"
#include "MyINIConfig.h"
#include "MyJSONConfig.h"
#include "MyLog.h"
#include "Pipeline.h"
#include "ServiceGuard.h"

namespace {

using json = nlohmann::json;

// 仅在信号处理函数里设置退出标记，真正的清理逻辑放回主线程中执行。
volatile std::sig_atomic_t g_exit_requested = 0;

// 统一维护默认路径和当前实际使用的路径，避免 main 中散落大量字符串常量。
struct AppPaths {
    std::string app_name = "fast_cpp_server";
    std::string default_ini_config_path = "/etc/fast_cpp_server/config.ini";
    std::string default_json_config_path = "/etc/fast_cpp_server/config.json";
    std::string default_yaml_config_path = "/etc/fast_cpp_server/config.yaml";
    std::string default_log_dir_path = "/var/fast_cpp_server/logs/";
    std::string default_log_file_path;

    std::string ini_config_path;
    std::string json_config_path;
    std::string yaml_config_path;
    std::string log_dir_path;
    std::string log_file_path;

    AppPaths()
        : default_log_file_path(default_log_dir_path + app_name + ".log"),
          ini_config_path(default_ini_config_path),
          json_config_path(default_json_config_path),
          yaml_config_path(default_yaml_config_path),
          log_dir_path(default_log_dir_path),
          log_file_path(default_log_file_path) {}
};

// 统一表达本次启动意图，main 只关心“要做什么”，不关心参数细节。
// 这些选项的默认值都设为 false，只有在明确传入参数时才会切换为 true。
struct StartupOptions {
    bool show_help      = false;    // 显示帮助信息后直接退出，不继续执行后续逻辑。
    bool show_version   = false;    // 显示版本信息后直接退出，不继续执行后续逻辑。
    bool run_setup      = false;    // 执行服务自检或守护逻辑。
    bool hard_setup     = false;    // 强制执行服务自检或守护逻辑。
    bool run_doctor     = false;    // 执行 MyDoctor 自检。
    std::string setup_mode;
    std::vector<std::map<std::string, std::string>> parsed_args;
};

// 启动期间产生的状态统一放在这里，方便按阶段输出详细中文日志。
struct BootstrapState {
    AppPaths paths;
    StartupOptions options;
    std::vector<std::string> bootstrap_logs;
    bool console_output = false;
    bool ini_loaded = false;
    bool json_path_from_ini = false;
    bool yaml_path_from_ini = false;
    bool json_loaded = false;
    bool yaml_loaded = false;
};

void HandleExitSignal(int signal_number) {
    (void)signal_number;
    g_exit_requested = 1;
}

bool IsOptionMatch(const std::map<std::string, std::string>& item,
                   const std::string& short_opt,
                   const std::string& long_opt) {
    const auto key_it = item.find("key");
    if (key_it == item.end()) {
        return false;
    }
    return key_it->second == short_opt || key_it->second == long_opt;
}

std::string GetOptionValue(const std::map<std::string, std::string>& item) {
    const auto value_it = item.find("value");
    return value_it == item.end() ? std::string() : value_it->second;
}

void AppendBootstrapLog(BootstrapState& state,
                        const std::string& message,
                        bool print_to_console = false) {
    if (print_to_console) {
        std::cout << message << std::endl;
    }
    state.bootstrap_logs.emplace_back(message);
}

ArgumentParser BuildArgumentParser() {
    ArgumentParser parser;
    parser.addOption("-h", "--help", "显示帮助信息");
    parser.addOption("-v", "--version", "显示版本信息");
    parser.addOption("-s", "--setup", "执行服务自检或守护逻辑", true);
    parser.addOption("-c", "--config", "指定 INI 配置文件路径", true);
    parser.addOption("-d", "--docter", "执行 MyDoctor 自检", false);
    return parser;
}

bool IsHardSetupMode(const std::string& value) {
    return value == "hard" || value == "true" || value == "1";
}

StartupOptions ParseStartupOptions(int argc, char* argv[], ArgumentParser& parser) {
    StartupOptions options;
    options.parsed_args = parser.parse(argc, argv);

    for (const auto& item : options.parsed_args) {
        if (IsOptionMatch(item, "-h", "--help")) {
            options.show_help = true;
        } else if (IsOptionMatch(item, "-v", "--version")) {
            options.show_version = true;
        } else if (IsOptionMatch(item, "-s", "--setup")) {
            options.run_setup = true;
            options.setup_mode = GetOptionValue(item);
            options.hard_setup = IsHardSetupMode(options.setup_mode);
        } else if (IsOptionMatch(item, "-d", "--docter")) {
            options.run_doctor = true;
        }
    }

    return options;
}

// 在日志系统初始化前，先把启动意图和原始参数收集起来，便于后续完整回放。
void LogArgumentOverview(BootstrapState& state) {
    AppendBootstrapLog(
        state,
        "[启动] 命令行解析完成，参数个数: " + std::to_string(state.options.parsed_args.size()),
        true);

    if (state.options.parsed_args.empty()) {
        AppendBootstrapLog(state, "[启动] 未检测到额外参数，将按默认流程启动。", true);
        return;
    }

    for (const auto& item : state.options.parsed_args) {
        const auto key_it = item.find("key");
        const auto value_it = item.find("value");
        const std::string key = key_it == item.end() ? std::string("<unknown>") : key_it->second;
        const std::string value = value_it == item.end() ? std::string() : value_it->second;
        const std::string message = value.empty()
            ? "[启动] 参数: " + key
            : "[启动] 参数: " + key + "，值: " + value;
        AppendBootstrapLog(state, message, true);
    }
}

// setup 模式属于“启动前动作”，单独前置执行，避免和正常服务启动路径缠在一起。
void ExecuteSetupIfRequested(BootstrapState& state) {
    if (!state.options.run_setup) {
        return;
    }

    AppendBootstrapLog(state, "[守护] 检测到 --setup 参数，开始执行服务自检/守护逻辑。", true);
    AppendBootstrapLog(state, "[守护] setup 原始参数值: " + state.options.setup_mode, true);
    AppendBootstrapLog(
        state,
        std::string("[守护] 当前判定的自检模式: ") + (state.options.hard_setup ? "硬自检" : "普通自检"),
        true);

    tools::service_guard::ServiceGuard::GetInstance().Execute(state.options.hard_setup);
    AppendBootstrapLog(state, "[守护] 服务自检/守护逻辑执行完成。", true);
}

// 先确定配置文件路径，再统一进入配置加载阶段，避免 main 中多处修改同一组路径变量。
void ResolveConfigPaths(BootstrapState& state) {
    AppendBootstrapLog(state, "[配置] 开始解析配置文件路径。", true);
    tools::init_tools::loadConfigFromArguments(
        state.options.parsed_args,
        state.bootstrap_logs,
        state.paths.ini_config_path);
    AppendBootstrapLog(state, "[配置] 最终 INI 配置路径: " + state.paths.ini_config_path, true);
}

void LoadAllConfigs(BootstrapState& state) {
    AppendBootstrapLog(state, "[配置] 开始依次加载 INI / JSON / YAML 配置。", true);

    state.ini_loaded = tools::init_tools::initLoadConfig(
        "ini",
        state.paths.ini_config_path,
        state.bootstrap_logs);

    state.json_path_from_ini = tools::init_tools::getJsonConfigPathFromINIConfig(
        state.paths.ini_config_path,
        state.paths.json_config_path,
        state.paths.default_json_config_path,
        state.bootstrap_logs);

    state.yaml_path_from_ini = tools::init_tools::getYamlConfigPathFromINIConfig(
        state.paths.ini_config_path,
        state.paths.yaml_config_path,
        state.paths.default_yaml_config_path,
        state.bootstrap_logs);

    state.json_loaded = tools::init_tools::initLoadConfig(
        "json",
        state.paths.json_config_path,
        state.bootstrap_logs);

    state.yaml_loaded = tools::init_tools::initLoadConfig(
        "yaml",
        state.paths.yaml_config_path,
        state.bootstrap_logs);

    AppendBootstrapLog(
        state,
        std::string("[配置] JSON 配置路径来源: ") +
            (state.json_path_from_ini ? "来自 INI 中的 config_dir" : "回退到默认路径"),
        true);
    AppendBootstrapLog(
        state,
        std::string("[配置] YAML 配置路径来源: ") +
            (state.yaml_path_from_ini ? "来自 INI 中的 config_dir" : "回退到默认路径"),
        true);
    AppendBootstrapLog(state, "[配置] JSON 配置路径: " + state.paths.json_config_path, true);
    AppendBootstrapLog(state, "[配置] YAML 配置路径: " + state.paths.yaml_config_path, true);
}

// 日志初始化刻意放在配置之后，这样可以优先使用配置中的日志目录和输出策略。
void InitializeLogger(BootstrapState& state, bool& logger_initialized) {
    AppendBootstrapLog(state, "[日志] 开始初始化日志系统。", true);

    if (!tools::free_func::loadLogConfigFromINIConfig(
            state.bootstrap_logs,
            state.paths.log_dir_path,
            state.paths.log_file_path,
            state.paths.app_name)) {
        state.paths.log_dir_path = state.paths.default_log_dir_path;
        state.paths.log_file_path = state.paths.default_log_file_path;
        AppendBootstrapLog(
            state,
            "[日志] 无法从 INI 中读取日志配置，已回退到默认日志文件: " + state.paths.log_file_path,
            true);
    }

    MyINIConfig::GetInstance().GetBool("console_output", false, state.console_output);
    AppendBootstrapLog(
        state,
        std::string("[日志] 控制台输出开关: ") + (state.console_output ? "开启" : "关闭"),
        true);
    AppendBootstrapLog(state, "[日志] 当前日志文件路径: " + state.paths.log_file_path, true);

    try {
        MyLog::Init(state.paths.log_file_path, 1048576 * 5, 3, state.console_output);
        logger_initialized = true;
    } catch (const std::exception& e) {
        AppendBootstrapLog(
            state,
            std::string("[日志] 日志初始化异常，已回退到默认控制台日志: ") + e.what(),
            true);
        logger_initialized = false;
    }
}

void DumpBootstrapLogs(const BootstrapState& state) {
    MYLOG_INFO("============================================================");
    MYLOG_INFO("以下为启动前阶段产生的详细日志：");
    for (const auto& item : state.bootstrap_logs) {
        MYLOG_INFO("{}", item);
    }
    MYLOG_INFO("============================================================");
}

int RunDoctorMode() {
    MYLOG_INFO("[自检] 进入 MyDoctor 自检模式。这个模式会执行内置环境检查并打印结果。");
    auto& doctor = my_doctor::MyDoctor::GetInstance();
    doctor.InitDefault();
    doctor.StartAll();

    const std::string result = doctor.ShowCheckResults();
    MYLOG_INFO("[自检] 自检完成，结果如下：\n{}", result);
    std::cout << result << std::endl;
    return 0;
}

bool ValidateConfigurationOrLogError(const BootstrapState& state) {
    if (tools::free_func::checkConfigLoadStatus(
            state.ini_loaded,
            state.json_loaded,
            state.yaml_loaded)) {
        MYLOG_INFO("[配置] 关键配置均已加载完成，可以继续启动应用。");
        return true;
    }

    MYLOG_ERROR("[配置] 关键配置加载失败，程序将提前退出。");
    return false;
}

void RegisterExitSignals() {
    std::signal(SIGINT, HandleExitSignal);
    std::signal(SIGTERM, HandleExitSignal);
}

void LogRuntimeSummary(const BootstrapState& state) {
    tools::free_func::logWelcomeMessage();

    MYLOG_INFO("[启动] 应用开始进入运行阶段。");
    MYLOG_INFO("[启动] 应用名称: {}", state.paths.app_name);
    MYLOG_INFO("[启动] 默认 INI 配置路径: {}", state.paths.default_ini_config_path);
    MYLOG_INFO("[启动] 默认 JSON 配置路径: {}", state.paths.default_json_config_path);
    MYLOG_INFO("[启动] 默认 YAML 配置路径: {}", state.paths.default_yaml_config_path);
    MYLOG_INFO("[启动] 当前 INI 配置路径: {}", state.paths.ini_config_path);
    MYLOG_INFO("[启动] 当前 JSON 配置路径: {}", state.paths.json_config_path);
    MYLOG_INFO("[启动] 当前 YAML 配置路径: {}", state.paths.yaml_config_path);
    MYLOG_INFO("[启动] 当前日志文件路径: {}", state.paths.log_file_path);
    MYLOG_INFO(
        "[启动] setup 模式: {}",
        state.options.run_setup ? (state.options.hard_setup ? "已启用硬自检" : "已启用普通自检") : "未启用");
    MYLOG_INFO("[启动] doctor 模式: {}", state.options.run_doctor ? "已请求" : "未请求");
    MYLOG_INFO("[启动] 命令行参数总数: {}", state.options.parsed_args.size());

    for (const auto& item : state.options.parsed_args) {
        const auto key_it = item.find("key");
        const auto value_it = item.find("value");
        const std::string key = key_it == item.end() ? std::string("<unknown>") : key_it->second;
        const std::string value = value_it == item.end() ? std::string() : value_it->second;
        MYLOG_INFO("[启动] 参数明细 -> key: {}, value: {}", key, value);
    }

    MYLOG_INFO("[启动] 已注册 SIGINT / SIGTERM 退出信号处理器。");
}

void ShowLoadedConfigs() {
    MYLOG_INFO("[配置] 输出当前加载到内存中的配置快照。");
    tools::free_func::showMyConfig("INI");
    tools::free_func::showMyConfig("JSON");
    tools::free_func::showMyConfig("YAML");
}

json LoadPipelineConfigFromJson() {
    json pipeline_config = json::object();
    MyJSONConfig::GetInstance().Get("pipeline", json::object(), pipeline_config);
    return pipeline_config;
}

void StartPipeline(const json& pipeline_config) {
    MYLOG_INFO("[Pipeline] 读取到的 pipeline 配置如下：\n{}", pipeline_config.dump(2));
    auto& pipeline = tools::pipeline::Pipeline::GetInstance();
    MYLOG_INFO("[Pipeline] 开始初始化 Pipeline。");
    pipeline.Init(pipeline_config);
    MYLOG_INFO("[Pipeline] Pipeline 初始化完成，准备启动所有已使能模块。");
    pipeline.Start();
    MYLOG_INFO("[Pipeline] Pipeline 已成功启动，应用进入常驻运行状态。");
}

// 主线程不再使用语义不明的死循环，而是显式表达“等待退出信号”。
void WaitForExitRequest() {
    MYLOG_INFO("[主循环] 主线程进入等待状态，按 Ctrl+C 或发送 SIGTERM 可触发优雅退出。");
    while (g_exit_requested == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    MYLOG_WARN("[主循环] 已收到退出信号，开始执行收尾流程。");
}

void StopPipeline() {
    MYLOG_INFO("[退出] 开始停止 Pipeline 及其关联模块。");
    tools::pipeline::Pipeline::GetInstance().Stop();
    MYLOG_INFO("[退出] Pipeline 停止完成。");
}

int RunApplication(int argc, char* argv[], bool& logger_initialized) {
    BootstrapState state;
    ArgumentParser parser = BuildArgumentParser();
    state.options = ParseStartupOptions(argc, argv, parser);

    // 第一阶段：优先处理不依赖任何运行时环境的轻量命令。
    if (state.options.show_help) {
        parser.printHelp();
        return 0;
    }
    if (state.options.show_version) {
        std::cout << "App Version: 1.0.0" << std::endl;
        return 0;
    }

    // 第二阶段：记录启动意图，并处理 setup 这类前置动作。
    LogArgumentOverview(state);
    ExecuteSetupIfRequested(state);

    // 第三阶段：解析配置路径并加载配置内容。
    ResolveConfigPaths(state);
    LoadAllConfigs(state);

    // 第四阶段：基于配置初始化日志系统，并把前面缓存的日志统一落盘。
    InitializeLogger(state, logger_initialized);
    DumpBootstrapLogs(state);

    // 第五阶段：doctor 模式保留独立出口，避免继续进入主业务启动。
    if (state.options.run_doctor) {
        const int doctor_result = RunDoctorMode();
        MyLog::Flush();
        return doctor_result;
    }

    // 第六阶段：确认配置完整，再进入真正的应用运行路径。
    if (!ValidateConfigurationOrLogError(state)) {
        MyLog::Flush();
        return -1;
    }

    RegisterExitSignals();
    LogRuntimeSummary(state);
    ShowLoadedConfigs();

    // 第七阶段：启动核心业务，并进入等待退出信号的常驻状态。
    const json pipeline_config = LoadPipelineConfigFromJson();
    StartPipeline(pipeline_config);
    WaitForExitRequest();

    // 第八阶段：执行优雅收尾，确保各模块有机会正常停止。
    StopPipeline();
    MYLOG_INFO("[退出] 程序已完成全部收尾流程，即将退出。");
    MyLog::Flush();
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    bool logger_initialized = false;

    try {
        return RunApplication(argc, argv, logger_initialized);
    } catch (const std::exception& e) {
        if (logger_initialized) {
            MYLOG_ERROR("[异常] 主程序发生未捕获异常: {}", e.what());
            MyLog::Flush();
        } else {
            std::cerr << "[异常] 主程序发生未捕获异常: " << e.what() << std::endl;
        }
        return -1;
    } catch (...) {
        if (logger_initialized) {
            MYLOG_ERROR("[异常] 主程序发生未知异常。");
            MyLog::Flush();
        } else {
            std::cerr << "[异常] 主程序发生未知异常。" << std::endl;
        }
        return -1;
    }
}