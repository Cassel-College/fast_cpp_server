/**
 * @file PodStreamManager.cpp
 * @brief 吊舱视频拉流守护模块 - 核心实现
 * 
 * 实现 PodStreamManager 的所有核心功能，包括:
 * - 双线程架构 (Monitor + Worker)
 * - 状态机管理
 * - 子进程生命周期管理 (fork/exec)
 * - 网络连通性探测 (ICMP ping)
 * - MediaMTX YAML 配置生成
 * 
 * @author peng.liu
 * @date 2026-04-16
 * @version 1.0.0
 */
#include "PodStreamManager.h"
#include "PodConfig.h"
#include "MyLog.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <cerrno>
#include <algorithm>

// POSIX 系统调用
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>  // for dirname()

namespace pod_stream {

// ============================================================================
// 路径工具函数 (替代 C++17 filesystem)
// ============================================================================

namespace {

/**
 * @brief 获取路径的父目录
 * @param path 文件路径
 * @return 父目录路径
 */
std::string GetParentPath(const std::string& path) {
    if (path.empty()) return ".";
    
    // 找到最后一个 '/'
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) {
        return ".";  // 没有目录分隔符，返回当前目录
    }
    if (pos == 0) {
        return "/";  // 根目录
    }
    return path.substr(0, pos);
}

/**
 * @brief 拼接路径
 * @param dir 目录
 * @param filename 文件名
 * @return 拼接后的完整路径
 */
std::string JoinPath(const std::string& dir, const std::string& filename) {
    if (dir.empty()) return filename;
    if (dir.back() == '/') {
        return dir + filename;
    }
    return dir + "/" + filename;
}

} // anonymous namespace


/**
 * @brief 获取当前时间戳字符串
 * @return 格式化的时间戳 [YYYY-MM-DD HH:MM:SS.mmm]
 */
std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
    localtime_r(&time_t_now, &tm_buf);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// ============================================================================
// PodStreamManager 单例与生命周期
// ============================================================================

PodStreamManager& PodStreamManager::GetInstance() {
    // C++11 函数内静态变量，天然线程安全
    static PodStreamManager instance;
    return instance;
}

PodStreamManager::~PodStreamManager() {
    // 确保退出时清理所有资源
    Stop();
}

const char* PodStreamManager::StateToString(State state) {
    switch (state) {
        case State::kIdle:     return "空闲";
        case State::kReady:    return "就绪";
        case State::kRunning:  return "运行中";
        case State::kStopping: return "停止中";
        case State::kError:    return "错误";
        default:               return "未知";
    }
}

PodStreamManager::State PodStreamManager::GetState() const {
    std::lock_guard<std::mutex> lock(state_mtx_);
    return state_;
}

// ============================================================================
// 初始化与停止
// ============================================================================

bool PodStreamManager::Init(const PodConfig& config) {
    std::string tempLog = "";
    // 防止重复初始化
    bool expected = false;
    if (!inited_.compare_exchange_strong(expected, true)) {
        tempLog = std::string("管理器已初始化，请勿重复调用 Init()");
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    tempLog = std::string("开始初始化吊舱流管理器...");
    MYLOG_INFO(tempLog.c_str());
    config_ = config;
    config_.Print();
    
    // 如果功能关闭，直接返回成功
    if (!config_.open) {
        tempLog = std::string("吊舱流媒体功能已禁用 (open=false)");
        MYLOG_INFO(tempLog.c_str());
        quit_.store(true);
        Transition(State::kIdle);
        return true;
    }
    
    // 验证配置
    if (!config_.IsValid()) {
        tempLog = std::string("初始化失败: 配置无效");
        MYLOG_INFO(tempLog.c_str());
        inited_.store(false);
        return false;
    }
    
    // 检查配置文件目录是否可写
    std::string config_dir = GetParentPath(config_.abs_mediamtx_path);
    if (access(config_dir.c_str(), W_OK) != 0) {
        tempLog = std::string("配置目录不可写: " + config_dir);
        MYLOG_INFO(tempLog.c_str());
        inited_.store(false);
        return false;
    }
    
    // 转换到 Ready 状态
    Transition(State::kReady);
    
    // 重置退出标志
    quit_.store(true);
    
    tempLog = std::string("吊舱流管理器初始化完成，等待 Start() 启动线程");
    MYLOG_INFO(tempLog.c_str());
    return true;
}


void PodStreamManager::Start() {
    MYLOG_INFO("PodStreamManager::Start() called");

    if (!inited_.load()) {
        MYLOG_ERROR("管理器尚未初始化，无法启动");
        return;
    }

    if (!config_.open) {
        MYLOG_INFO("吊舱流媒体功能已禁用，忽略 Start()");
        return;
    }

    if (monitor_th_.joinable() || worker_th_.joinable()) {
        MYLOG_WARN("管理器线程已启动，忽略重复 Start()");
        return;
    }

    quit_.store(false);

    monitor_th_ = std::thread(&PodStreamManager::MonitorLoop, this);
    MYLOG_INFO("监控线程已启动");

    worker_th_ = std::thread(&PodStreamManager::WorkerLoop, this);
    MYLOG_INFO("工作线程已启动");

    MYLOG_INFO("PodStreamManager 已启动");

}


void PodStreamManager::Stop() {
    if (!inited_.load()) {
        return;
    }
    std::string tempLog = "";
    
    tempLog = std::string("正在停止吊舱流管理器...");
    MYLOG_INFO(tempLog.c_str());
    
    // 设置退出标志
    quit_.store(true);
    
    // 等待线程结束
    if (monitor_th_.joinable()) {
        monitor_th_.join();
        tempLog = std::string("监控线程已停止");
        MYLOG_INFO(tempLog.c_str());
    }
    
    if (worker_th_.joinable()) {
        worker_th_.join();
        tempLog = std::string("工作线程已停止");
        MYLOG_INFO(tempLog.c_str());
    }
    
    // 确保子进程被清理
    if (IsMediaMtxAlive()) {
        tempLog = std::string("正在清理 MediaMTX 子进程...");
        MYLOG_INFO(tempLog.c_str());
        StopMediaMtx(false);  // 先尝试优雅停止
        
        // 如果仍然存活，强制杀死
        if (IsMediaMtxAlive()) {
            StopMediaMtx(true);
        }
    }
    
    // 回收僵尸进程
    ReapChildIfNeeded();
    
    Transition(State::kIdle);
    inited_.store(false);
    
    tempLog = std::string("吊舱流管理器已停止");
    MYLOG_INFO(tempLog.c_str());
}

// ============================================================================
// 状态机管理
// ============================================================================

void PodStreamManager::Transition(State next) {
    std::lock_guard<std::mutex> lock(state_mtx_);
    std::string tempLog = "";
    if (state_ != next) {
        tempLog = std::string(std::string("状态转换: ") + StateToString(state_) + 
              " -> " + StateToString(next));
        MYLOG_INFO(tempLog.c_str());
        state_ = next;
    }
}

std::string PodStreamManager::GetStatusString() const {
    State state = GetState();
    bool connected = is_connected_.load();
    pid_t pid = mediamtx_pid_.load();
    
    std::ostringstream oss;
    oss << "状态: " << StateToString(state)
        << ", 网络: " << (connected ? "已连接" : "未连接")
        << ", MediaMTX PID: " << (pid > 0 ? std::to_string(pid) : "无");
    return oss.str();

}

void PodStreamManager::RecordCrash() {
    std::string tempLog = "";
    std::lock_guard<std::mutex> lock(crash_mtx_);
    auto now = std::chrono::steady_clock::now();
    crash_timestamps_.push_back(now);
    
    // 清理超出时间窗口的记录
    auto window_start = now - std::chrono::seconds(config_.crash_window_seconds);
    crash_timestamps_.erase(
        std::remove_if(crash_timestamps_.begin(), crash_timestamps_.end(),
            [window_start](const std::chrono::steady_clock::time_point& t) { 
                return t < window_start; 
            }),
        crash_timestamps_.end());
    
    // 增加退避等级
    tempLog = std::string("增加退避等级, 当前等级: " + std::to_string(backoff_level_) + ", 最大等级 kMaxBackoffLevel: " + std::to_string(kMaxBackoffLevel));
    MYLOG_INFO(tempLog.c_str());
    if (backoff_level_ < kMaxBackoffLevel) {
        backoff_level_ = backoff_level_ + 1;
        tempLog = std::string("退避等级已增加到 " + std::to_string(backoff_level_));
        MYLOG_INFO(tempLog.c_str());
    } else {
        tempLog = std::string("退避等级已达到最大值 " + std::to_string(kMaxBackoffLevel));
        MYLOG_INFO(tempLog.c_str());
    }
    
    tempLog = std::string("记录崩溃事件, 时间窗口内崩溃次数: " + 
          std::to_string(crash_timestamps_.size()) +
          ", 退避等级: " + std::to_string(backoff_level_));
    MYLOG_INFO(tempLog.c_str());
}

bool PodStreamManager::IsCrashingTooOften() const {
    std::lock_guard<std::mutex> lock(crash_mtx_);
    if (config_.max_crash_count < 0) {
        return false;  // 不限制崩溃次数
    } else {
        return static_cast<int>(crash_timestamps_.size()) >= config_.max_crash_count;
    }
}

int PodStreamManager::GetBackoffDelayMs() const {
    std::lock_guard<std::mutex> lock(crash_mtx_);
    int delayMs = 1000;
    if (backoff_level_ == 0) return 0;
    // 指数退避: 1s, 2s, 4s, 8s, 10s
    if (backoff_level_ <= 1) {
        delayMs = 1000;
    } else if (backoff_level_ <= 2) {
        delayMs = 2000;
    } else if (backoff_level_ <= 3) {
        delayMs = 4000;
    } else if (backoff_level_ <= 4) {
        delayMs = 8000;
    } else {
        delayMs = 10000;  // 超过最大退避等级，固定在 10s
    }
    return delayMs;
}

void PodStreamManager::ResetBackoff() {
    std::lock_guard<std::mutex> lock(crash_mtx_);
    std::string tempLog = "";
    backoff_level_ = 0;
    tempLog = std::string("退避计数器已重置");
    MYLOG_INFO(tempLog.c_str());
}

// ============================================================================
// Monitor Thread - 连通性监控
// ============================================================================

void PodStreamManager::MonitorLoop() {
    std::string tempLog = "";
    tempLog = std::string("监控线程启动 - 正在监控 " + config_.pod_ip);
    MYLOG_INFO(tempLog.c_str());
    int local_success_count = 0;
    int local_fail_count = 0;
    bool last_probe_result = false;
    
    while (!quit_.load()) {
        // 探测连通性
        bool probe_ok = ProbeConnectivityOnce();
        
        if (probe_ok) {
            local_fail_count = 0;
            ++local_success_count;
            
            // 去抖: 连续成功 N 次才认为 connected
            if (!is_connected_.load() && 
                local_success_count >= config_.connect_debounce_count) {
                is_connected_.store(true);
                tempLog = std::string("网络已连通 " + config_.pod_ip + 
                      " (经过 " + std::to_string(local_success_count) + " 次探测)");
                MYLOG_INFO(tempLog.c_str());
            }
        } else {
            local_success_count = 0;
            ++local_fail_count;
            
            // 去抖: 连续失败 N 次才认为 disconnected
            if (is_connected_.load() && 
                local_fail_count >= config_.disconnect_debounce_count) {
                is_connected_.store(false);
                tempLog = std::string("网络已断开 " + config_.pod_ip + 
                      " (连续 " + std::to_string(local_fail_count) + " 次失败)");
                MYLOG_INFO(tempLog.c_str());
            }
        }
        
        // 更新计数器 (供外部调试查看)
        connect_success_count_.store(local_success_count);
        connect_fail_count_.store(local_fail_count);
        
        // 调试日志 (仅在状态变化时输出)
        if (probe_ok != last_probe_result) {
            tempLog = std::string(std::string("探测结果变化: ") + 
                  (probe_ok ? "成功" : "失败"));
            MYLOG_INFO(tempLog.c_str());
            last_probe_result = probe_ok;
        }
        
        // 休眠等待下一次探测
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.monitor_period_ms));
    }
    
    tempLog = std::string("监控线程已退出");
    MYLOG_INFO(tempLog.c_str());
}

bool PodStreamManager::ProbeConnectivityOnce() {
    // 使用 ping 命令探测 (ICMP)
    // -c 1: 发送 1 个包
    // -W 1: 超时 1 秒
    // -q: 静默模式
    std::string cmd = "ping -c 1 -W 1 -q " + config_.pod_ip + " > /dev/null 2>&1";
    
    int ret = system(cmd.c_str());
    return (ret == 0);
}

// ============================================================================
// Worker Thread - 状态机调度
// ============================================================================

void PodStreamManager::WorkerLoop() {
    std::string tempLog = "";
    tempLog = std::string("工作线程启动");
    MYLOG_INFO(tempLog.c_str());
    
    int status_log_count = 0;
    while (!quit_.load()) {
        // 回收可能存在的僵尸子进程
        bool child_exited = ReapChildIfNeeded();
        
        State current_state = GetState();
        bool connected = is_connected_.load();
        
        switch (current_state) {
            case State::kIdle:
                // 空闲状态，不做任何操作
                break;
                
            case State::kReady:
                if (connected) {
                    // 检查冷却时间
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - last_stop_time_).count();
                    
                    if (elapsed < kCooldownMs) {
                        tempLog = std::string("冷却中，等待 " + 
                              std::to_string(kCooldownMs - elapsed) + " 毫秒");
                        MYLOG_INFO(tempLog.c_str());
                        break;
                    }
                    
                    // 检查退避时间
                    int backoff_ms = GetBackoffDelayMs();
                    if (backoff_ms > 0) {
                        tempLog = std::string("退避等待中，等待 " + std::to_string(backoff_ms) + " 毫秒");
                        MYLOG_INFO(tempLog.c_str());
                        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                        
                        // 休眠后再次检查状态
                        if (quit_.load() || !is_connected_.load()) {
                            break;
                        }
                    }
                    
                    // 准备配置并启动 MediaMTX
                    tempLog = std::string("网络已连通，准备启动 MediaMTX...");
                    MYLOG_INFO(tempLog.c_str());
                    
                    if (!PrepareMediaMtxConfig()) {
                        tempLog = std::string("准备 MediaMTX 配置失败，保持就绪状态");
                        MYLOG_INFO(tempLog.c_str());
                        RecordCrash();  // 配置失败也算一次"崩溃"
                        break;
                    }
                    
                    if (!StartMediaMtx()) {
                        tempLog = std::string("启动 MediaMTX 失败，保持就绪状态");
                        MYLOG_INFO(tempLog.c_str());
                        RecordCrash();
                        break;
                    }
                    
                    // 启动成功，重置退避
                    // ResetBackoff();
                    Transition(State::kRunning);
                }
                break;
                
            case State::kRunning:
                if (!connected) {
                    // 网络断开，停止 MediaMTX
                    tempLog = std::string("网络已断开，正在停止 MediaMTX...");
                    MYLOG_INFO(tempLog.c_str());
                    Transition(State::kStopping);
                    StopMediaMtx(false);
                    last_stop_time_ = std::chrono::steady_clock::now();
                    Transition(State::kReady);
                } else if (child_exited) {
                    // 子进程意外退出
                    tempLog = std::string("MediaMTX 进程意外退出");
                    MYLOG_INFO(tempLog.c_str());
                    RecordCrash();
                    
                    if (IsCrashingTooOften()) {
                        tempLog = std::string("崩溃过于频繁，进入错误状态");
                        MYLOG_INFO(tempLog.c_str());
                        Transition(State::kError);
                    } else {
                        // 尝试重启
                        tempLog = std::string("即将尝试重启 MediaMTX...");
                        MYLOG_INFO(tempLog.c_str());
                        Transition(State::kReady);
                    }
                } else if (!IsMediaMtxAlive()) {
                    // 进程不存在 (可能已被外部杀死)
                    tempLog = std::string("MediaMTX 进程已不存在");
                    MYLOG_INFO(tempLog.c_str());
                    RecordCrash();
                    Transition(State::kReady);
                }
                break;
                
            case State::kStopping:
                // 等待停止完成
                if (!IsMediaMtxAlive()) {
                    ReapChildIfNeeded();
                    Transition(State::kReady);
                }
                break;
                
            case State::kError:
                // 错误状态，等待一段时间后尝试恢复
                if (!IsCrashingTooOften()) {
                    tempLog = std::string("崩溃频率降低，返回就绪状态");
                    MYLOG_INFO(tempLog.c_str());
                    Transition(State::kReady);
                }
                break;
        }
        
        
        status_log_count = status_log_count + 1;
        if (status_log_count % 10 == 0) {
            status_log_count = 0;
            std::string status_info = GetStatusString();
            MYLOG_INFO(status_info.c_str());
        }
        

        // 休眠等待下一次调度
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.worker_period_ms));
    }
    
    tempLog = std::string("工作线程已退出");
    MYLOG_INFO(tempLog.c_str());
}

// ============================================================================
// 配置管理 - MediaMTX YAML 生成
// ============================================================================

std::string PodStreamManager::GetConfigPath() const {
    std::string parent_dir = GetParentPath(config_.abs_mediamtx_path);
    return JoinPath(parent_dir, "mediamtx.yml");
}

std::string PodStreamManager::GetLogFilePath() const {
    std::string parent_dir = GetParentPath(config_.abs_mediamtx_path);
    return JoinPath(parent_dir, "mediamtx.log");
}

std::string PodStreamManager::GenerateYamlContent() const {
    std::ostringstream oss;
    
    // YAML 头部配置
    oss << "# MediaMTX Configuration\n";
    oss << "# Auto-generated by PodStreamManager\n";
    oss << "# Generated at: " << GetTimestamp() << "\n";
    oss << "\n";
    
    // RTSP 服务配置
    oss << "rtspAddress: :" << config_.rtsp_port << "\n";
    oss << "\n";
    
    // 流路径配置
    oss << "paths:\n";
    
    for (const auto& kv : config_.stream_mapping) {
        const std::string& output_path = kv.first;
        const std::string& input_url = kv.second;
        oss << "  " << output_path << ":\n";
        oss << "    source: " << input_url << "\n";
        // oss << "    sourceOnDemand: yes\n";
        oss << "    sourceOnDemand: no\n";
    }
    return oss.str();
}

bool PodStreamManager::PrepareMediaMtxConfig() {
    std::string tempLog = "";
    std::string config_path = GetConfigPath();
    std::string yaml_content = GenerateYamlContent();
    
    tempLog = std::string("正在准备 MediaMTX 配置: " + config_path);
    MYLOG_INFO(tempLog.c_str());
    tempLog = std::string("YAML 内容:\n" + yaml_content);
    MYLOG_INFO(tempLog.c_str());
    
    // 原子写入: 先写临时文件，再重命名
    std::string tmp_path = config_path + ".tmp";
    
    // 写入临时文件
    {
        std::ofstream ofs(tmp_path);
        if (!ofs.is_open()) {
            tempLog = std::string("无法打开临时配置文件: " + tmp_path);
            MYLOG_INFO(tempLog.c_str());
            return false;
        }
        
        ofs << yaml_content;
        ofs.flush();
        
        if (ofs.fail()) {
            tempLog = std::string("写入临时配置文件失败: " + tmp_path);
            MYLOG_INFO(tempLog.c_str());
            ofs.close();
            unlink(tmp_path.c_str());
            return false;
        }
        
        ofs.close();
    }
    
    // 重命名临时文件为正式文件 (原子操作)
    if (rename(tmp_path.c_str(), config_path.c_str()) != 0) {
        tempLog = std::string("重命名临时配置文件失败: " + 
              std::string(strerror(errno)));
        MYLOG_INFO(tempLog.c_str());
        unlink(tmp_path.c_str());
        return false;
    }
    
    tempLog = std::string("MediaMTX 配置写入成功: " + config_path);
    MYLOG_INFO(tempLog.c_str());
    return true;
}

// ============================================================================
// 子进程管理 - MediaMTX 启动/停止
// ============================================================================

bool PodStreamManager::StartMediaMtx() {
    std::string tempLog = "";
    // 检查是否已经在运行
    if (IsMediaMtxAlive()) {
        tempLog = std::string("MediaMTX 已在运行中 (pid=" + std::to_string(mediamtx_pid_.load()) + ")");
        MYLOG_INFO(tempLog.c_str());
        return true;
    }
    
    std::string config_path = GetConfigPath();
    std::string log_path = GetLogFilePath();
    
    tempLog = std::string("正在启动 MediaMTX: " + config_.abs_mediamtx_path);
    MYLOG_INFO(tempLog.c_str());
    tempLog = std::string("  配置文件: " + config_path);
    MYLOG_INFO(tempLog.c_str());
    tempLog = std::string("  日志文件: " + log_path);
    MYLOG_INFO(tempLog.c_str());
    
    // fork 子进程
    pid_t pid = fork();
    
    if (pid < 0) {
        // fork 失败
        tempLog = std::string("fork() 失败: " + std::string(strerror(errno)));
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    if (pid == 0) {
        // 子进程
        
        // 创建新的会话 (进程组 leader)
        setsid();
        
        // 重定向 stdout 和 stderr 到日志文件
        int log_fd = open(log_path.c_str(), 
                          O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            close(log_fd);
        }
        
        // 关闭 stdin
        close(STDIN_FILENO);
        
        // 构建命令行参数
        // mediamtx [config_file]
        char* argv[] = {
            const_cast<char*>(config_.abs_mediamtx_path.c_str()),
            const_cast<char*>(config_path.c_str()),
            nullptr
        };
        
        // 执行 MediaMTX
        execv(config_.abs_mediamtx_path.c_str(), argv);
        
        // 如果 execv 返回，说明出错了
        std::cerr << "execv() 失败: " << strerror(errno) << std::endl;
        _exit(127);
    }
    
    // 父进程
    mediamtx_pid_.store(pid);
    tempLog = std::string("MediaMTX 已启动，pid=" + std::to_string(pid));
    MYLOG_INFO(tempLog.c_str());
    
    // 短暂等待，确认进程确实启动了
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (!IsMediaMtxAlive()) {
        tempLog = std::string("MediaMTX 进程启动后立即退出");
        MYLOG_INFO(tempLog.c_str());
        ReapChildIfNeeded();
        return false;
    }
    
    return true;
}

void PodStreamManager::StopMediaMtx(bool force_kill) {
    std::string tempLog = "";
    pid_t pid = mediamtx_pid_.load();
    if (pid <= 0) {
        tempLog = std::string("没有需要停止的 MediaMTX 进程");
        MYLOG_INFO(tempLog.c_str());
        return;
    }
    
    if (!IsMediaMtxAlive()) {
        tempLog = std::string("MediaMTX 进程已经结束");
        MYLOG_INFO(tempLog.c_str());
        ReapChildIfNeeded();
        return;
    }
    
    if (force_kill) {
        // 强制杀死
        tempLog = std::string("发送 SIGKILL 到 MediaMTX (pid=" + std::to_string(pid) + ")");
        MYLOG_INFO(tempLog.c_str());
        kill(pid, SIGKILL);
    } else {
        // 优雅停止: 先 SIGTERM，超时后 SIGKILL
        tempLog = std::string("发送 SIGTERM 到 MediaMTX (pid=" + std::to_string(pid) + ")");
        MYLOG_INFO(tempLog.c_str());
        kill(pid, SIGTERM);
        
        // 等待进程退出
        auto start_time = std::chrono::steady_clock::now();
        int timeout_ms = config_.graceful_stop_timeout_ms;
        
        while (IsMediaMtxAlive()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            
            if (elapsed >= timeout_ms) {
                tempLog = std::string("SIGTERM 超时，发送 SIGKILL (pid=" + std::to_string(pid) + ")");
                MYLOG_INFO(tempLog.c_str());
                kill(pid, SIGKILL);
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // 等待进程完全退出并回收
    int status = 0;
    int wait_attempts = 0;
    const int max_wait_attempts = 30;  // 最多等待 3 秒
    
    while (wait_attempts < max_wait_attempts) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            // 子进程已退出
            if (WIFEXITED(status)) {
                tempLog = std::string("MediaMTX 正常退出，退出码: " + std::to_string(WEXITSTATUS(status)));
                MYLOG_INFO(tempLog.c_str());
            } else if (WIFSIGNALED(status)) {
                tempLog = std::string("MediaMTX 被信号终止，信号: " + std::to_string(WTERMSIG(status)));
                MYLOG_INFO(tempLog.c_str());
            }
            mediamtx_pid_.store(-1);
            return;
        } else if (result < 0) {
            // 已经没有子进程了
            mediamtx_pid_.store(-1);
            return;
        }
        
        ++wait_attempts;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    tempLog = std::string("等待 MediaMTX 退出超时，强制清理");
    MYLOG_INFO(tempLog.c_str());
    mediamtx_pid_.store(-1);
}

bool PodStreamManager::IsMediaMtxAlive() const {
    pid_t pid = mediamtx_pid_.load();
    if (pid <= 0) {
        return false;
    }
    
    // 使用 kill(pid, 0) 检查进程是否存在
    // 返回 0 表示进程存在且有权限
    // 返回 -1 且 errno == ESRCH 表示进程不存在
    if (kill(pid, 0) == 0) {
        return true;
    }
    
    return (errno != ESRCH);
}

bool PodStreamManager::ReapChildIfNeeded() {
    std::string tempLog = "";
    pid_t pid = mediamtx_pid_.load();
    if (pid <= 0) {
        return false;
    }
    
    int status = 0;
    pid_t result = waitpid(pid, &status, WNOHANG);
    
    if (result == pid) {
        // 子进程已退出
        std::ostringstream oss;
        oss << "已回收 MediaMTX 进程 (pid=" << pid << ")";
        
        if (WIFEXITED(status)) {
            oss << " 退出码=" << WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            oss << " 信号=" << WTERMSIG(status);
            if (WCOREDUMP(status)) {
                oss << " (core dump)";
            }
        }
        
        tempLog = std::string(oss.str());
        MYLOG_INFO(tempLog.c_str());
        mediamtx_pid_.store(-1);
        return true;
    }
    
    return false;
}

} // namespace pod_stream

