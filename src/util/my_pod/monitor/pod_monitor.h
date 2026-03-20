#pragma once

/**
 * @file pod_monitor.h
 * @brief 吊舱后台监控器
 *
 * PodMonitor 在独立线程中定期轮询各项能力模块，将采集结果
 * 聚合到 PodRuntimeStatus 数据对象中。外部读取时直接获取
 * 快照，无需实时查询设备。
 *
 * 设计要点：
 * - 轮询顺序：依次按 状态→云台→激光→流媒体 执行
 * - 在线判定：使用滑动窗口，需连续多次检测以确定最终状态
 * - 错误隔离：每个能力的轮询均用 try-catch 保护，
 *   单个能力异常不影响其余能力和主循环
 * - 可中断关闭：主循环睡眠分片，确保 stop() 快速返回
 */

#include "../common/pod_models.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <cstdint>

namespace PodModule {

class IPod;  // 前向声明，避免循环依赖

class PodMonitor {
public:
    PodMonitor();
    ~PodMonitor();

    // 不可拷贝/移动
    PodMonitor(const PodMonitor&) = delete;
    PodMonitor& operator=(const PodMonitor&) = delete;

    /**
     * @brief 启动监控线程
     * @param pod  关联的 Pod 实例（非拥有，生命周期由调用方保证）
     * @param config 轮询配置
     */
    void start(IPod* pod, const PodMonitorConfig& config = {});

    /** @brief 停止监控线程（阻塞至线程退出） */
    void stop();

    /** @brief 是否正在运行 */
    bool isRunning() const;

    /** @brief 获取运行时状态快照（线程安全） */
    PodRuntimeStatus getRuntimeStatus() const;

    /** @brief 更新轮询配置（线程安全，下一轮循环生效） */
    void updateConfig(const PodMonitorConfig& config);

private:
    /** @brief 监控主循环 */
    void monitorLoop();

    /** @brief 轮询状态/在线检测 */
    void pollStatus();

    /** @brief 轮询云台姿态 */
    void pollPtz();

    /** @brief 轮询激光数据 */
    void pollLaser();

    /** @brief 轮询流媒体状态 */
    void pollStream();

    /** @brief 更新在线判定滑动窗口 */
    void updateOnlineWindow(bool ping_result);

    /** @brief 获取当前时间戳（毫秒，steady_clock） */
    static uint64_t nowMs();

    IPod*               pod_ = nullptr;
    PodMonitorConfig    config_;
    PodRuntimeStatus    runtime_status_;

    mutable std::mutex  status_mutex_;      // 保护 runtime_status_ + online_history_
    mutable std::mutex  config_mutex_;      // 保护 config_

    std::thread         monitor_thread_;
    std::atomic<bool>   running_{false};

    std::deque<bool>    online_history_;     // 在线检测滑动窗口
};

} // namespace PodModule
