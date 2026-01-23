#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h> // for sleep function

#include <spdlog/spdlog.h>

// #include <easylogging++.h>
// INITIALIZE_EASYLOGGINGPP

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

#include "ArgumentParser.h"
#include "MyLog.h"
#include "MyINIConfig.h"
#include "MyJSONConfig.h"
#include "MyYAMLConfig.h"
#include "MyHeartbeatManager.h"
#include "MyAPI.h"

// #include "system_healthy.h"
#include "MySystemHealthyManager.h"

#include "InitTools.h"
#include "FreeFunc.h"
#include "Pipeline.h"
#include "SimpleTcpClient.h"
#include "ThreadSafeBuffer.h"
#include "device_online_monitor/DeviceOnlineMonitor.h"
#include "ServiceGuard.h"
#include "MyDoctor.h"


using namespace tools;
using namespace my_api;
using namespace my_doctor;
// using namespace MySystemHealthy;
using json = nlohmann::json;



int main(int argc, char* argv[]) {

    std::string appName                     = "fast_cpp_server";
    std::string defaultINIConfigFilePath    = "/etc/fast_cpp_server/config.ini";
    std::string defaultJSONConfigFilePath   = "/etc/fast_cpp_server/config.json";
    std::string defaultYAMLConfigFilePath   = "/etc/fast_cpp_server/config.yaml";
    std::string defaultLogDirPath           = "/var/fast_cpp_server/logs/";
    std::string defaultLogFilePath          = defaultLogDirPath + appName + ".log";
    std::string configFilePath              = defaultINIConfigFilePath;
    std::string logDirPath                  = defaultLogDirPath;
    std::string logFilePath                 = defaultLogFilePath;
    std::vector<std::string> logInfos       = {};
    bool is_hard_setup                      = false; // 是否强制自检

    ArgumentParser parser;
    parser.addOption("-h", "--help",    "Show help info");
    parser.addOption("-v", "--version", "Show version info");
    parser.addOption("-s", "--setup",   "Setup mode",       true);
    parser.addOption("-c", "--config",  "Set config file",  true);
    parser.addOption("-d", "--docter",  "Run MyDoctor self-check", false);

    std::vector<std::map<std::string, std::string>> args = parser.parse(argc, argv);
    for (const auto& item : args) {
        if (item.at("key") == "-h" || item.at("key") == "--help") {
            parser.printHelp();
            return 0;
        } else if (item.at("key") == "-v" || item.at("key") == "--version") {
            std::cout << "App Version: 1.0.0" << std::endl;
            return 0;
        } else if (item.at("key") == "-s" || item.at("key") == "--setup") {
            std::cout << "Setup mode activated: mode:" << item.at("value") << std::endl;
            std::string value = item.at("value");
            if (value == "hard" || value == "true" || value == "1") {
                is_hard_setup = true;
                std::cout << "Hard setup mode activated." << std::endl;
                logInfos.emplace_back("Hard setup mode activated.");
            } else {
                std::cout << "Normal setup mode activated." << std::endl;
                logInfos.emplace_back("Normal setup mode activated.");
            }
            tools::service_guard::ServiceGuard::GetInstance().Execute(is_hard_setup);       // 执行服务自检
        } else if (item.at("key") == "-c" || item.at("key") == "--config") {
            tools::init_tools::loadConfigFromArguments(args, logInfos, configFilePath);
        } else {
            // Unknown argument, can log or ignore
        }
    }

    // 加载配置文件
    bool load_ini_config_status  = tools::init_tools::initLoadConfig("ini",  configFilePath,  logInfos);
    bool load_json_config_status = tools::init_tools::initLoadConfig("json", defaultJSONConfigFilePath, logInfos);
    bool load_yaml_config_status = tools::init_tools::initLoadConfig("yaml", defaultYAMLConfigFilePath, logInfos);
    bool console_output          = false;
    // 初始化日志系统
    if (!tools::free_func::loadLogConfigFromINIConfig(logInfos, logDirPath, logFilePath, appName)) {
        logDirPath = defaultLogDirPath;
        logFilePath = logDirPath + appName + ".log";
        std::cout << "[LogConfig] 使用默认日志目录: " << logDirPath << std::endl;
        logInfos.emplace_back("[LogConfig] 使用默认日志目录: " + logDirPath);
    }
    MyINIConfig::GetInstance().GetBool("console_output", false, console_output);
    logInfos.emplace_back("[LogConfig] 日志是否输出到控制台: " + std::string(console_output ? "是" : "否"));

    MyLog::Init(logFilePath, 1048576 * 5, 3, console_output);  // ✅ 只调用一次
    
    for (const auto& item : args) {
        if (item.at("key") == "-d" || item.at("key") == "--docter") {
            MYLOG_INFO("Starting MyDoctor self-check...");
            my_doctor::MyDoctor& doctor = my_doctor::MyDoctor::GetInstance();
            doctor.InitDefault();
            doctor.StartAll();
            std::cout << doctor.ShowCheckResults() << std::endl;
            return 0;
        }
    }

    MYLOG_INFO("----------------------------------- Init Log -------------------------");
    if (!tools::free_func::checkConfigLoadStatus(load_ini_config_status, load_json_config_status, load_yaml_config_status)) {
        MYLOG_ERROR("配置加载失败，程序退出.");
        return -1;
    }
    for (std::string& logItem : logInfos) {
        MYLOG_INFO(logItem);
    }
    MYLOG_INFO("----------------------------------------------------------------------");

    tools::free_func::logWelcomeMessage();
    MYLOG_INFO("App is starting...");
    MYLOG_INFO(" * defaultINIConfigFilePath: {}", defaultINIConfigFilePath);
    MYLOG_INFO(" * defaultJSONConfigFilePath: {}", defaultJSONConfigFilePath);
    MYLOG_INFO(" * defaultYAMLConfigFilePath: {}", defaultYAMLConfigFilePath);
    MYLOG_INFO(" * logFilePath: {}", logFilePath);
    MYLOG_INFO("----------------------------------------------------------------------");
    MYLOG_INFO("Argument parsing completed. Parsed {} arguments.", args.size());
    for (const auto& item : args) {
        MYLOG_INFO("* Arg: {}, Value: {}", item.at("key"), item.at("value"));
    }
    MYLOG_INFO("----------------------------------------------------------------------");

    tools::free_func::showMyConfig("INI");
    tools::free_func::showMyConfig("JSON");
    tools::free_func::showMyConfig("YAML");

    // my_api::RunRestServer(8964);

    // 1. 读取配置文件得到 json 对象
    nlohmann::json pipelineConfig = {};
    MyJSONConfig::GetInstance().Get("pipeline", {}, pipelineConfig);
    MYLOG_INFO("pipeline Config: \n {}", pipelineConfig.dump(2));
    MYLOG_INFO("======================================================================");
    // 2. 初始化 Pipeline (单例)
    tools::pipeline::Pipeline::GetInstance().Init(pipelineConfig);
    // 3. 启动所有使能的连接
    tools::pipeline::Pipeline::GetInstance().Start();

    // 主线程保持运行
    bool break_loop = false;
    while (true) {
        // 获取推出信号
        if (break_loop) { break;} 
        sleep(3);
    }
    // 4. 停止并清理
    tools::pipeline::Pipeline::GetInstance().Stop();

    
    MySystemHealthy::MySystemHealthyManager::GetInstance().Init(5);

    // cpr::Response r = cpr::Get(cpr::Url{"https://api.github.com/users/octocat"});

    // // 检查请求是否成功
    // if (r.status_code == 200) {
    //     // 输出响应内容
    //     std::cout << "Request successful!" << std::endl;
    //     std::cout << "Status code: " << r.status_code << std::endl;
    //     std::cout << "Response body: " << r.text << std::endl;
    // } else {
    //     // 输出错误信息
    //     std::cout << "Request failed with status code: " << r.status_code << std::endl;
    //     std::cout << "Error message: " << r.error.message << std::endl;
    // }

    std::string ip = "192.168.1.230";
    int port = 3000;

    // std::string ip = "192.168.1.166";
    // int port = 5000;

    int maxTimeoutMs = 3000;
    SimpleTcpClient simpleTcpClient = SimpleTcpClient(ip, port, maxTimeoutMs);
    std::vector<char> outData = {};

    return 0;
}