/**
 * @file RtspRelayMonitor.cpp
 * @brief RTSP 转发守护模块 —— 实现
 *
 * 核心流程：
 *   Init()  → 解析配置 → 生成 YAML
 *   Start() → 启动 MonitorLoop 线程
 *   MonitorLoop() → 周期检测 RTSP 源 → 管理 MediaMTX 进程
 *   Stop()  → 终止进程 → 回收线程
 */

#include "RtspRelayMonitor.h"

#include <sstream>
#include <chrono>
#include <cstring>
#include <cerrno>

#include "MyFileTools.h"

// Linux 系统头文件
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>

#include "MyLog.h"
#include "MyIPTools.h"
#include "PingTools.h"
#include "MyStartDir.h"

namespace my_mediamtx_monitor {

// ============================================================================
//  辅助：状态枚举 → 可读字符串
// ============================================================================

const char* RtspRelayStateToString(RtspRelayState state) {
    switch (state) {
        case RtspRelayState::STOPPED:     return "STOPPED";
        case RtspRelayState::STARTING:    return "STARTING";
        case RtspRelayState::RUNNING:     return "RUNNING";
        case RtspRelayState::SOURCE_LOST: return "SOURCE_LOST";
        case RtspRelayState::ERROR:       return "ERROR";
        default:                          return "UNKNOWN";
    }
}

// ============================================================================
//  构造 / 析构
// ============================================================================

RtspRelayMonitor::RtspRelayMonitor() {
    MYLOG_INFO("[RtspRelayMonitor] 实例已创建");
}

RtspRelayMonitor::~RtspRelayMonitor() {
    Stop();
    MYLOG_INFO("[RtspRelayMonitor] 实例已销毁");
}

// ============================================================================
//  Init —— 配置解析 + YAML 生成
// ============================================================================

bool RtspRelayMonitor::Init(const nlohmann::json& json_config) {
    std::lock_guard<std::mutex> lock(mutex_);
    MYLOG_INFO("[RtspRelayMonitor] 开始初始化，配置: {}", json_config.dump(4));

    json_config_ = json_config;

    // ---- 解析配置字段 ----
    if (!ParseConfig()) {
        MYLOG_ERROR("[RtspRelayMonitor] 配置解析失败，初始化终止");
        return false;
    }

    local_ip_ = my_tools::MyIPTools::GetLocalIPv4();
    std::string root_path = my_tools::MyStartDir().getStartDir();
    if (root_path.empty()) {
        MYLOG_ERROR("[RtspRelayMonitor] 启动目录为空，无法生成 YAML 文件路径");
        return false;
    }
    if (yaml_file_name_.empty()) {
        MYLOG_ERROR("[RtspRelayMonitor] yaml_file_name 为空，无法生成 YAML 文件路径");
        return false;
    }
    yaml_file_abs_path_ = root_path + "/" + yaml_file_name_;

    // ---- 生成 YAML 文件 ----
    if (!GenerateYAML()) {
        MYLOG_ERROR("[RtspRelayMonitor] YAML 文件生成失败，初始化终止");
        return false;
    }

    config_initialized_ = true;

    // ---- 收集基本信息字段 ----
    info_ = nlohmann::json::object();
    info_["module"]              = "RtspRelayMonitor";
    info_["enabled"]             = enable_.load();

    // 源信息
    nlohmann::json source;
    source["check_ip"]           = check_ip_;
    source["check_port"]         = check_port_;
    if (mediamtx_json_config_.contains("paths") && mediamtx_json_config_["paths"].is_object()) {
        for (auto& [name, path_cfg] : mediamtx_json_config_["paths"].items()) {
            nlohmann::json s;
            s["stream_name"] = name;
            if (path_cfg.contains("source") && path_cfg["source"].is_string()) {
                s["source_url"] = path_cfg["source"].get<std::string>();
            }
            if (path_cfg.contains("sourceOnDemand") && path_cfg["sourceOnDemand"].is_boolean()) {
                s["on_demand"] = path_cfg["sourceOnDemand"].get<bool>();
            }
            source["streams"].push_back(s);
        }
    }
    info_["source"] = source;

    // 本机信息
    nlohmann::json local;
    local["rtsp_listen_port"]       = rtsp_listen_port_;
    local["rtsp_listen_address"]    = local_ip_ + ":" + std::to_string(rtsp_listen_port_);
    local["mediamtx_bin"]           = mediamtx_bin_;
    local["yaml_file"]              = yaml_file_abs_path_;
    info_["local"]                  = local;
    


    // 拉流方式
    nlohmann::json pull;
    pull["protocol"]            = "RTSP";
    pull["relay_url"]           = "rtsp://" + local_ip_ + ":" + std::to_string(rtsp_listen_port_) + "/live";
    pull["description"]         = "通过 RTSP 协议从本机转发端口拉取视频流";
    info_["pull_method"] = pull;

    // 监控参数
    info_["monitor_interval_sec"] = monitor_interval_sec_;

    // 帮助信息
    nlohmann::json help;
    help["usage"]    = "本模块守护 MediaMTX 进程，周期探测 RTSP 源连通性，自动启停转发";
    help["api"]      = nlohmann::json::array({
        "/v1/mediamtx/online  — 存活检查",
        "/v1/mediamtx/status  — 运行状态与心跳",
        "/v1/mediamtx/info    — 转发代理基本信息",
        "/v1/mediamtx/start   — 启动监控",
        "/v1/mediamtx/stop    — 停止监控"
    });
    info_["help"] = help;

    MYLOG_INFO("[RtspRelayMonitor] 初始化成功 | enable={} | check_ip={}:{} | listen_port={} | yaml={}",
               enable_.load(), check_ip_, check_port_, rtsp_listen_port_, yaml_file_abs_path_);
    return true;
}

// ============================================================================
//  ParseConfig —— 从 JSON 提取各项参数
// ============================================================================

bool RtspRelayMonitor::ParseConfig() {
    try {
        // 1) mediamtx_enable（默认 false）
        if (json_config_.contains("mediamtx_enable") && json_config_["mediamtx_enable"].is_boolean()) {
            enable_ = json_config_["mediamtx_enable"].get<bool>();
        }
        MYLOG_INFO("[RtspRelayMonitor] mediamtx_enable = {}", enable_.load());

        // 2) check_ip（必须）
        if (!json_config_.contains("check_ip") || !json_config_["check_ip"].is_string()) {
            MYLOG_ERROR("[RtspRelayMonitor] 缺少必要字段 'check_ip'");
            return false;
        }
        check_ip_ = json_config_["check_ip"].get<std::string>();

        // 3) check_port（可选，默认 554）
        if (json_config_.contains("check_port") && json_config_["check_port"].is_number_integer()) {
            check_port_ = json_config_["check_port"].get<int>();
        }

        // 4) yaml_file_name（可选）
        if (json_config_.contains("yaml_file_name") && json_config_["yaml_file_name"].is_string()) {
            yaml_file_name_ = json_config_["yaml_file_name"].get<std::string>();
        }

        // 5) mediamtx_bin_path（可选，默认 ./mediamtx）
        //    兼容旧字段名 "mediamtx_bin"
        if (json_config_.contains("mediamtx_bin_path") && json_config_["mediamtx_bin_path"].is_string()) {
            mediamtx_bin_ = json_config_["mediamtx_bin_path"].get<std::string>();
        } else if (json_config_.contains("mediamtx_bin") && json_config_["mediamtx_bin"].is_string()) {
            mediamtx_bin_ = json_config_["mediamtx_bin"].get<std::string>();
        }

        // 6) monitor_interval_sec（可选，默认 5 秒）
        if (json_config_.contains("monitor_interval_sec") && json_config_["monitor_interval_sec"].is_number_integer()) {
            monitor_interval_sec_ = json_config_["monitor_interval_sec"].get<int>();
            if (monitor_interval_sec_ < 1) monitor_interval_sec_ = 1;
        }

        // 7) mediamtx_json_config（必须）
        if (!json_config_.contains("mediamtx_json_config") || !json_config_["mediamtx_json_config"].is_object()) {
            MYLOG_ERROR("[RtspRelayMonitor] 缺少必要字段 'mediamtx_json_config'");
            return false;
        }
        mediamtx_json_config_ = json_config_["mediamtx_json_config"];

        // 8) 从 mediamtx_json_config 提取监听端口（仅接受纯端口字符串，如 "8555"）
        if (mediamtx_json_config_.contains("rtspAddress") && mediamtx_json_config_["rtspAddress"].is_string()) {
            std::string addr = mediamtx_json_config_["rtspAddress"].get<std::string>();
            try {
                rtsp_listen_port_ = std::stoi(addr);
            } catch (...) {
                MYLOG_WARN("[RtspRelayMonitor] 解析 rtspAddress '{}' 端口失败，使用默认 {}", addr, rtsp_listen_port_);
            }
        }

        MYLOG_INFO("[RtspRelayMonitor] 配置解析完成 | check_ip={}:{} | listen_port={} | bin={} | interval={}s",
                   check_ip_, check_port_, rtsp_listen_port_, mediamtx_bin_, monitor_interval_sec_);
        return true;

    } catch (const std::exception& e) {
        MYLOG_ERROR("[RtspRelayMonitor] 配置解析异常: {}", e.what());
        return false;
    }
}

// ============================================================================
//  GenerateYAML —— JSON → YAML 字符串 → 写入文件
// ============================================================================

/**
 * @brief 递归地将 nlohmann::json 转为 YAML 格式字符串
 * @param j       当前 JSON 节点
 * @param indent  缩进层级
 * @return YAML 文本片段
 */
std::string JsonToYaml(const nlohmann::json& j, int indent) {
    std::ostringstream oss;
    std::string pad(indent * 2, ' ');

    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            if (it.value().is_object()) {
                // 对象类型：key + 换行 + 递归
                oss << pad << it.key() << ":\n" << JsonToYaml(it.value(), indent + 1);
            } else if (it.value().is_array()) {
                // 数组类型
                oss << pad << it.key() << ":\n";
                for (const auto& elem : it.value()) {
                    if (elem.is_object() || elem.is_array()) {
                        oss << pad << "  -\n" << JsonToYaml(elem, indent + 2);
                    } else {
                        oss << pad << "  - " << elem.dump() << "\n";
                    }
                }
            } else if (it.value().is_string()) {
                // 字符串值加引号
                oss << pad << it.key() << ": \"" << it.value().get<std::string>() << "\"\n";
            } else if (it.value().is_boolean()) {
                // 布尔值
                oss << pad << it.key() << ": " << (it.value().get<bool>() ? "yes" : "no") << "\n";
            } else {
                // 数字或其他
                oss << pad << it.key() << ": " << it.value().dump() << "\n";
            }
        }
    }
    return oss.str();
}

bool RtspRelayMonitor::GenerateYAML() {
    try {
        // 生成 YAML 内容
        yaml_content_ = JsonToYaml(mediamtx_json_config_);
        MYLOG_INFO("[RtspRelayMonitor] 生成的 YAML 内容:\n{}", yaml_content_);

        if (yaml_file_abs_path_.empty()) {
            MYLOG_ERROR("[RtspRelayMonitor] YAML 文件路径为空，无法写入");
            return false;
        }

        // 如果文件已存在则删除（使用 MyFileTools）
        if (my_tools::MyFileTools::Exists(yaml_file_abs_path_)) {
            if (my_tools::MyFileTools::DeleteFile(yaml_file_abs_path_)) {
                MYLOG_INFO("[RtspRelayMonitor] 已删除旧 YAML 文件: {}", yaml_file_abs_path_);
            } else {
                MYLOG_WARN("[RtspRelayMonitor] 删除旧 YAML 文件失败: {}", yaml_file_abs_path_);
            }
        }

        // 写入文件（使用 MyFileTools 覆盖写入）
        if (!my_tools::MyFileTools::WriteText(yaml_file_abs_path_, yaml_content_)) {
            MYLOG_ERROR("[RtspRelayMonitor] 无法写入 YAML 文件: {}", yaml_file_abs_path_);
            return false;
        }

        MYLOG_INFO("[RtspRelayMonitor] YAML 文件已写入: {}", yaml_file_abs_path_);
        return true;

    } catch (const std::exception& e) {
        MYLOG_ERROR("[RtspRelayMonitor] 生成 YAML 异常: {}", e.what());
        return false;
    }
}

// ============================================================================
//  Start / Stop
// ============================================================================

void RtspRelayMonitor::Start() {
    if (!config_initialized_) {
        MYLOG_ERROR("[RtspRelayMonitor] 尚未成功初始化，无法启动监控");
        return;
    }

    if (running_.load()) {
        MYLOG_WARN("[RtspRelayMonitor] 监控线程已在运行中，忽略重复启动");
        return;
    }

    running_ = true;
    monitor_thread_ = std::thread(&RtspRelayMonitor::MonitorLoop, this);
    MYLOG_INFO("[RtspRelayMonitor] 监控线程已启动");
}

void RtspRelayMonitor::Stop() {
    MYLOG_INFO("[RtspRelayMonitor] 停止请求...");

    // 1) 置运行标志为 false，令 MonitorLoop 退出
    running_ = false;

    // 2) 终止 MediaMTX 子进程
    StopRelayProcess();

    // 3) 等待监控线程退出
    if (monitor_thread_.joinable()) {
        if (std::this_thread::get_id() != monitor_thread_.get_id()) {
            monitor_thread_.join();
            MYLOG_INFO("[RtspRelayMonitor] 监控线程已退出");
        } else {
            MYLOG_WARN("[RtspRelayMonitor] Stop() 在监控线程内部调用，跳过 self-join");
        }
    }

    state_ = RtspRelayState::STOPPED;
    MYLOG_INFO("[RtspRelayMonitor] 已完全停止");
}

// ============================================================================
//  MonitorLoop —— 核心守护循环
// ============================================================================

void RtspRelayMonitor::MonitorLoop() {
    MYLOG_INFO("[RtspRelayMonitor] 监控循环启动 | 轮询间隔 {}s", monitor_interval_sec_);

    while (running_.load()) {
        // ---- 休眠（分段睡眠，以便快速响应 Stop） ----
        for (int i = 0; i < monitor_interval_sec_ && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!running_.load()) break;

        // ---- 未启用 → 停止进程 ----
        if (!enable_.load()) {
            MYLOG_DEBUG("[RtspRelayMonitor] mediamtx_enable=false，停止转发进程");
            StopRelayProcess();
            state_ = RtspRelayState::STOPPED;
            continue;
        }

        // ---- RTSP 源不可达 → 停止进程 ----
        if (!CheckRtspSource()) {
            MYLOG_WARN("[RtspRelayMonitor] RTSP 源 {}:{} 不可达，停止转发进程", check_ip_, check_port_);
            StopRelayProcess();
            state_ = RtspRelayState::SOURCE_LOST;
            continue;
        }

        // ---- 进程存活检测 ----
        if (!CheckProcessAlive()) {
            MYLOG_INFO("[RtspRelayMonitor] MediaMTX 进程未运行，尝试启动...");
            if (StartRelayProcess()) {
                state_ = RtspRelayState::STARTING;
                MYLOG_INFO("[RtspRelayMonitor] MediaMTX 进程启动成功，PID={}", relay_pid_);
            } else {
                state_ = RtspRelayState::ERROR;
                MYLOG_ERROR("[RtspRelayMonitor] MediaMTX 进程启动失败");
            }
        } else {
            state_ = RtspRelayState::RUNNING;
            MYLOG_DEBUG("[RtspRelayMonitor] MediaMTX 进程运行中，PID={}", relay_pid_);
        }
    }

    MYLOG_INFO("[RtspRelayMonitor] 监控循环已退出");
}

// ============================================================================
//  CheckRtspSource —— TCP socket 探测 RTSP 源
// ============================================================================

bool RtspRelayMonitor::CheckRtspSource() {
    // const bool reachable = my_tools::ping_tools::PingFuncBySystem::PingIPBySocket(check_ip_, check_port_, 2);
    const bool reachable = my_tools::ping_tools::PingFuncBySystem::PingIP(check_ip_, 1, 2);
    if (reachable) {
        MYLOG_DEBUG("[RtspRelayMonitor] RTSP 源 {}可达", check_ip_);
        return true;
    }
    MYLOG_DEBUG("[RtspRelayMonitor] RTSP 源 {}不可达", check_ip_);
    return false;
}

// ============================================================================
//  CheckProcessAlive —— 检测子进程是否存活
// ============================================================================

bool RtspRelayMonitor::CheckProcessAlive() {
    if (relay_pid_ <= 0) {
        return false;
    }

    // kill(pid, 0)：不发送信号，仅检查进程是否存在
    if (::kill(relay_pid_, 0) == 0) {
        return true;
    }

    // 进程已退出，尝试回收以避免僵尸进程
    int status = 0;
    pid_t w = ::waitpid(relay_pid_, &status, WNOHANG);
    if (w > 0) {
        MYLOG_WARN("[RtspRelayMonitor] MediaMTX 进程 PID={} 已退出，exit_status={}", relay_pid_, WEXITSTATUS(status));
    }
    relay_pid_ = -1;
    return false;
}

// ============================================================================
//  StartRelayProcess —— fork + exec 启动 MediaMTX
// ============================================================================

bool RtspRelayMonitor::StartRelayProcess() {
    try {
        // 前置检查：端口是否可用
        if (!IsPortAvailable(rtsp_listen_port_)) {
            MYLOG_WARN("[RtspRelayMonitor] 端口 {} 已被占用，暂不启动 MediaMTX", rtsp_listen_port_);
            return false;
        }

        // 检查 YAML 文件是否存在
        if (!my_tools::MyFileTools::Exists(yaml_file_abs_path_)) {
            MYLOG_ERROR("[RtspRelayMonitor] YAML 配置文件不存在: {}", yaml_file_abs_path_);
            return false;
        }

        MYLOG_INFO("[RtspRelayMonitor] 启动 MediaMTX: {} {}", mediamtx_bin_, yaml_file_abs_path_);

        pid_t pid = ::fork();
        if (pid < 0) {
            MYLOG_ERROR("[RtspRelayMonitor] fork 失败: {}", std::strerror(errno));
            return false;
        }

        if (pid == 0) {
            // ---- 子进程 ----
            // 重定向 stdout/stderr 到 /dev/null（MediaMTX 日志由其自身管理）
            int devnull = ::open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                ::dup2(devnull, STDOUT_FILENO);
                ::dup2(devnull, STDERR_FILENO);
                ::close(devnull);
            }

            // exec 替换为 MediaMTX 进程
            ::execlp(mediamtx_bin_.c_str(), "mediamtx", yaml_file_abs_path_.c_str(), nullptr);

            // execlp 失败时退出子进程
            _exit(127);
        }

        // ---- 父进程 ----
        relay_pid_ = pid;
        MYLOG_INFO("[RtspRelayMonitor] MediaMTX 子进程已创建，PID={}", relay_pid_);
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("[RtspRelayMonitor] StartRelayProcess 异常: {}", e.what());
        return false;
    } catch (...) {
        MYLOG_ERROR("[RtspRelayMonitor] StartRelayProcess 未知异常");
        return false;
    }
}

// ============================================================================
//  StopRelayProcess —— 优雅终止进程
// ============================================================================

void RtspRelayMonitor::StopRelayProcess() {
    if (relay_pid_ <= 0) {
        return;
    }

    pid_t pid = relay_pid_;
    MYLOG_INFO("[RtspRelayMonitor] 正在停止 MediaMTX 进程，PID={}", pid);

    // 1) 发送 SIGTERM 请求优雅退出
    ::kill(pid, SIGTERM);

    // 2) 轮询等待进程退出（最多 5 秒）
    constexpr int kTimeoutSec = 5;
    for (int i = 0; i < kTimeoutSec; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (::kill(pid, 0) != 0) {
            MYLOG_INFO("[RtspRelayMonitor] MediaMTX 进程 PID={} 已正常退出", pid);
            break;
        }
    }

    // 3) 超时仍存活 → SIGKILL 强制终止
    if (::kill(pid, 0) == 0) {
        MYLOG_WARN("[RtspRelayMonitor] MediaMTX PID={} 超时未退出，发送 SIGKILL", pid);
        ::kill(pid, SIGKILL);
    }

    // 4) waitpid 回收子进程，避免僵尸
    int status = 0;
    pid_t w = ::waitpid(pid, &status, 0);
    if (w == pid) {
        MYLOG_INFO("[RtspRelayMonitor] waitpid 回收 MediaMTX PID={}，status={}", pid, status);
    } else {
        MYLOG_WARN("[RtspRelayMonitor] waitpid 回收 PID={} 失败，返回值={}", pid, w);
    }

    relay_pid_ = -1;
}

// ============================================================================
//  IsPortAvailable —— 检测端口是否空闲
// ============================================================================

bool RtspRelayMonitor::IsPortAvailable(int port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        MYLOG_WARN("[RtspRelayMonitor] 端口检测创建 socket 失败: {}", std::strerror(errno));
        return false;
    }

    int opt = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    int ret = ::bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    ::close(sock);

    if (ret == 0) {
        MYLOG_DEBUG("[RtspRelayMonitor] 端口 {} 可用", port);
        return true;
    }
    MYLOG_DEBUG("[RtspRelayMonitor] 端口 {} 已被占用", port);
    return false;
}

// ============================================================================
//  GetState / GetHeartbeat
// ============================================================================

RtspRelayState RtspRelayMonitor::GetState() const {
    return state_;
}

nlohmann::json RtspRelayMonitor::GetHeartbeat() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json info;
    info["module"]           = "RtspRelayMonitor";
    info["state"]            = RtspRelayStateToString(state_);
    info["enable"]           = enable_.load();
    info["check_ip"]         = check_ip_;
    info["check_port"]       = check_port_;
    info["rtsp_listen_port"] = rtsp_listen_port_;
    info["relay_pid"]        = relay_pid_;
    info["yaml_file"]        = yaml_file_abs_path_;
    return info;
}

nlohmann::json RtspRelayMonitor::GetInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

} // namespace my_mediamtx_monitor
