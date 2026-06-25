/**
 * @file PodStreamManager.hpp
 * @brief 吊舱视频拉流守护模块 - 头文件定义
 * 
 * PodStreamManager 用于在嵌入式/机载域控环境中，持续监控吊舱网络连通性，
 * 并根据连通状态动态管理 MediaMTX 推流/转发进程，实现"网络可用即自动开流、
 * 网络不可用即自动停流/清理"的闭环能力。
 * 
 * @author peng.liu
 * @date 2026-04-16
 * @version 1.0.0
 */

#ifndef POD_STREAM_MANAGER_HPP
#define POD_STREAM_MANAGER_HPP

#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <sys/types.h>

#include "PodConfig.h"


namespace pod_stream {

// ============================================================================
// PodStreamManager 核心类
// ============================================================================

/**
 * @brief 吊舱视频流管理器 (单例模式)
 * 
 * 设计原则:
 * - 单例: 全局唯一管理者，避免多实例竞争进程控制权
 * - 线程安全: 并发读写的状态使用原子变量/互斥锁保障一致性
 * - 鲁棒: 网络抖动、进程崩溃、配置写失败等异常可自恢复
 * - 可观测: 关键状态变化、启动/停止动作、错误信息必须日志化
 * 
 * 双线程架构:
 * - Monitor Thread: 周期探测吊舱连通性，维护 is_connected 状态
 * - Worker Thread: 根据连通状态驱动状态机，管理 MediaMTX 子进程
 */
class PodStreamManager final {
public:
    /**
     * @brief 状态机状态枚举
     */
    enum class State {
        kIdle,     ///< 空闲状态，未初始化或已停止
        kReady,    ///< 就绪状态，配置有效，等待网络连通
        kRunning,  ///< 运行状态，MediaMTX 进程正在运行
        kStopping, ///< 停止中，正在停止 MediaMTX 进程
        kError     ///< 错误状态，需要人工干预或等待恢复
    };
    
    /**
     * @brief 获取单例实例
     * @return 管理器单例引用
     * 
     * 使用 C++11 函数内静态变量实现懒加载单例，天然线程安全。
     */
    static PodStreamManager& GetInstance();
    
    /**
     * @brief 唯一初始化入口
     * @param config 吊舱配置
     * @return 初始化是否成功
     * 
     * 完成参数校验、配置加载/改写、线程启动。
     * 若已初始化，重复调用将返回 false。
     */
    bool Init(const PodConfig& config);
    
    /**
     * @brief 启动管理器
     * 
     * 启动监控线程和工作线程，开始管理 MediaMTX 子进程。
     */
    void Start();

    /**
     * @brief 停止管理器
     * 
     * 优雅停止所有线程和子进程，便于进程退出时优雅收尾。
     */
    void Stop();
    
    /**
     * @brief 检查是否已初始化
     * @return 是否已初始化
     */
    bool IsInited() const { return inited_.load(); }
    
    /**
     * @brief 获取当前状态
     * @return 当前状态机状态
     */
    State GetState() const;
    
    /**
     * @brief 获取状态名称字符串
     * @param state 状态枚举
     * @return 状态名称
     */
    static const char* StateToString(State state);
    
    /**
     * @brief 检查网络连通状态
     * @return 是否连通
     */
    bool IsConnected() const { return is_connected_.load(); }
    
    /**
     * @brief 获取 MediaMTX 进程 PID
     * @return PID，若未运行则返回 -1
     */
    pid_t GetMediaMtxPid() const { return mediamtx_pid_.load(); }


    /**
     * @brief 获取当前状态字符串 (调试用)
     * @return 当前状态字符串
     */
    std::string GetStatusString() const;
private:
    // ========== 构造/析构 (禁止外部实例化) ==========
    PodStreamManager() = default;
    ~PodStreamManager();
    
    // ========== 禁止拷贝和移动 ==========
    PodStreamManager(const PodStreamManager&) = delete;
    PodStreamManager& operator=(const PodStreamManager&) = delete;
    PodStreamManager(PodStreamManager&&) = delete;
    PodStreamManager& operator=(PodStreamManager&&) = delete;
    
    // ========== 核心线程函数 ==========
    
    /**
     * @brief 监控线程主循环
     * 
     * 周期性探测吊舱 IP 连通性，更新 is_connected 状态。
     * 使用去抖逻辑避免网络抖动导致状态频繁切换。
     */
    void MonitorLoop();
    
    /**
     * @brief 工作线程主循环 (状态机调度)
     * 
     * 根据 is_connected 状态驱动状态机转换，
     * 管理 MediaMTX 子进程的启动、监控和停止。
     */
    void WorkerLoop();
    
    // ========== 连通性探测 ==========
    
    /**
     * @brief 单次连通性探测
     * @return 是否连通
     * 
     * 使用 ICMP ping 探测吊舱 IP 是否可达。
     */
    bool ProbeConnectivityOnce();
    
    // ========== 配置管理 ==========
    
    /**
     * @brief 准备 MediaMTX 配置文件
     * @return 是否成功
     * 
     * 根据 stream_mapping 生成或更新 MediaMTX YAML 配置文件。
     * 配置文件位于 abs_mediamtx_path 同级目录。
     */
    bool PrepareMediaMtxConfig();
    
    /**
     * @brief 获取 MediaMTX 配置文件路径
     * @return 配置文件绝对路径
     */
    std::string GetConfigPath() const;
    
    /**
     * @brief 生成 YAML 配置内容
     * @return YAML 字符串
     */
    std::string GenerateYamlContent() const;
    
    // ========== 子进程管理 ==========
    
    /**
     * @brief 启动 MediaMTX 子进程
     * @return 是否启动成功
     * 
     * 使用 fork/exec 启动子进程，记录 PID。
     * 子进程的 stdout/stderr 重定向到日志文件。
     */
    bool StartMediaMtx();
    
    /**
     * @brief 停止 MediaMTX 子进程
     * @param force_kill 是否强制杀死 (跳过 SIGTERM)
     * 
     * 两阶段停止: SIGTERM 优雅退出 -> 超时后 SIGKILL 强制结束。
     */
    void StopMediaMtx(bool force_kill = false);
    
    /**
     * @brief 检查 MediaMTX 进程是否存活
     * @return 是否存活
     */
    bool IsMediaMtxAlive() const;
    
    /**
     * @brief 回收子进程，避免僵尸进程
     * @return 是否回收了子进程 (子进程已退出)
     */
    bool ReapChildIfNeeded();
    
    /**
     * @brief 获取日志文件路径 (用于子进程输出重定向)
     * @return 日志文件路径
     */
    std::string GetLogFilePath() const;
    
    // ========== 状态机 ==========
    
    /**
     * @brief 状态转换
     * @param next 目标状态
     */
    void Transition(State next);
    
    /**
     * @brief 记录崩溃事件，用于崩溃频率统计
     */
    void RecordCrash();
    
    /**
     * @brief 检查是否处于崩溃过频状态
     * @return 是否崩溃过频
     */
    bool IsCrashingTooOften() const;
    
    /**
     * @brief 获取重试退避时间 (指数退避)
     * @return 退避时间 (毫秒)
     */
    int GetBackoffDelayMs() const;
    
    /**
     * @brief 重置退避计数器
     */
    void ResetBackoff();

private:
    // ========== 配置 ==========
    PodConfig config_;
    
    // ========== 状态标志 ==========
    std::atomic<bool> inited_{false};     ///< 是否已初始化
    std::atomic<bool> quit_{false};       ///< 退出标志，通知线程退出
    
    // ========== 连通性状态 (Monitor Thread 写, Worker Thread 读) ==========
    std::atomic<bool> is_connected_{false};
    
    // ========== 子进程状态 ==========
    std::atomic<pid_t> mediamtx_pid_{-1};
    
    // ========== 状态机 ==========
    mutable std::mutex state_mtx_;
    State state_{State::kIdle};
    
    // ========== 线程对象 ==========
    std::thread monitor_th_;
    std::thread worker_th_;
    
    // ========== 去抖计数器 ==========
    std::atomic<int> connect_success_count_{0};
    std::atomic<int> connect_fail_count_{0};
    
    // ========== 崩溃统计与退避 ==========
    mutable std::mutex crash_mtx_;
    std::vector<std::chrono::steady_clock::time_point> crash_timestamps_;
    int backoff_level_{0};  ///< 退避等级 (0=无退避, 1=1s, 2=2s, ...)
    std::chrono::steady_clock::time_point last_stop_time_;  ///< 上次停止时间
    static constexpr int kMaxBackoffLevel = 5;  ///< 最大退避等级 (2^5 = 32秒)
    static constexpr int kCooldownMs = 2000;    ///< 停止后冷却时间 (毫秒)
};

} // namespace pod_stream

#endif // POD_STREAM_MANAGER_HPP
