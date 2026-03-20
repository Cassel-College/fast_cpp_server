/**
 * @file pod_monitor.cpp
 * @brief 吊舱后台监控器实现
 */

#include "pod_monitor.h"
#include "../pod/interface/i_pod.h"
#include "../capability/interface/i_status_capability.h"
#include "../capability/interface/i_ptz_capability.h"
#include "../capability/interface/i_laser_capability.h"
#include "../capability/interface/i_stream_capability.h"
#include "MyLog.h"
#include <chrono>
#include <algorithm>

namespace PodModule {

// ==================== 生命周期 ====================

PodMonitor::PodMonitor() {
    MYLOG_DEBUG("[PodMonitor] 构造");
}

PodMonitor::~PodMonitor() {
    stop();
    MYLOG_DEBUG("[PodMonitor] 析构");
}

// ==================== 启动 / 停止 ====================

void PodMonitor::start(IPod* pod, const PodMonitorConfig& config) {
    if (running_.load()) {
        MYLOG_WARN("[PodMonitor] 已在运行中，忽略重复启动");
        return;
    }
    if (!pod) {
        MYLOG_ERROR("[PodMonitor] Pod 为空，无法启动监控");
        return;
    }

    pod_ = pod;
    {
        std::lock_guard<std::mutex> lk(config_mutex_);
        config_ = config;
    }
    {
        std::lock_guard<std::mutex> lk(status_mutex_);
        runtime_status_ = PodRuntimeStatus{};
        online_history_.clear();
    }

    running_.store(true);
    monitor_thread_ = std::thread(&PodMonitor::monitorLoop, this);

    MYLOG_INFO("[PodMonitor] 监控线程已启动 (pod={}, poll={}ms, status={}ms, ptz={}ms)",
               pod_->getPodId(), config.poll_interval_ms,
               config.status_interval_ms, config.ptz_interval_ms);
}

void PodMonitor::stop() {
    if (!running_.load()) return;

    running_.store(false);
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    MYLOG_INFO("[PodMonitor] 监控线程已停止");
}

bool PodMonitor::isRunning() const {
    return running_.load();
}

// ==================== 数据读取 / 配置更新 ====================

PodRuntimeStatus PodMonitor::getRuntimeStatus() const {
    std::lock_guard<std::mutex> lk(status_mutex_);
    return runtime_status_;
}

void PodMonitor::updateConfig(const PodMonitorConfig& config) {
    std::lock_guard<std::mutex> lk(config_mutex_);
    config_ = config;
    MYLOG_INFO("[PodMonitor] 轮询配置已更新");
}

// ==================== 工具方法 ====================

uint64_t PodMonitor::nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count());
}

// ==================== 监控主循环 ====================

void PodMonitor::monitorLoop() {
    MYLOG_INFO("[PodMonitor] 监控循环开始 (pod={})", pod_->getPodId());

    uint64_t last_status_time = 0;
    uint64_t last_ptz_time    = 0;
    uint64_t last_laser_time  = 0;
    uint64_t last_stream_time = 0;

    while (running_.load()) {
        // 读取当前配置快照
        PodMonitorConfig cfg;
        {
            std::lock_guard<std::mutex> lk(config_mutex_);
            cfg = config_;
        }

        const uint64_t now = nowMs();

        // ---- 按顺序依次轮询各能力 ----

        // 1) 状态 / 在线检测
        if (cfg.enable_status_poll && (now - last_status_time >= cfg.status_interval_ms)) {
            pollStatus();
            last_status_time = nowMs();
        }

        // 2) 云台姿态
        if (cfg.enable_ptz_poll && (now - last_ptz_time >= cfg.ptz_interval_ms)) {
            pollPtz();
            last_ptz_time = nowMs();
        }

        // 3) 激光测距
        if (cfg.enable_laser_poll && (now - last_laser_time >= cfg.laser_interval_ms)) {
            pollLaser();
            last_laser_time = nowMs();
        }

        // 4) 流媒体状态
        if (cfg.enable_stream_poll && (now - last_stream_time >= cfg.stream_interval_ms)) {
            pollStream();
            last_stream_time = nowMs();
        }

        // ---- 可中断睡眠：分片 100ms，快速响应 stop() ----
        const uint32_t step_ms = 100;
        for (uint32_t slept = 0; slept < cfg.poll_interval_ms && running_.load(); slept += step_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                std::min(step_ms, cfg.poll_interval_ms - slept)));
        }
    }

    MYLOG_INFO("[PodMonitor] 监控循环结束 (pod={})", pod_->getPodId());
}

// ==================== 各能力轮询（try-catch 保护） ====================

void PodMonitor::pollStatus() {
    try {
        auto cap = pod_->getCapabilityAs<IStatusCapability>(CapabilityType::STATUS);
        if (!cap) return;

        // 先刷新，再读取
        cap->refreshStatus();
        auto result = cap->getStatus();

        bool ping_ok = false;
        if (result.isSuccess() && result.data.has_value()) {
            ping_ok = (result.data->state == PodState::CONNECTED);
        }

        // 更新滑动窗口（内部会更新 is_online）
        updateOnlineWindow(ping_ok);

        // 写入运行时状态
        std::lock_guard<std::mutex> lk(status_mutex_);
        if (result.isSuccess() && result.data.has_value()) {
            runtime_status_.pod_status = result.data.value();
        }
        // 用滑动窗口结果覆盖 state
        runtime_status_.pod_status.state = runtime_status_.is_online
            ? PodState::CONNECTED : PodState::DISCONNECTED;
        runtime_status_.online_update_ms = nowMs();
        runtime_status_.last_update_ms   = runtime_status_.online_update_ms;

    } catch (const std::exception& e) {
        MYLOG_ERROR("[PodMonitor] 状态轮询异常: {}", e.what());
        updateOnlineWindow(false);
    } catch (...) {
        MYLOG_ERROR("[PodMonitor] 状态轮询未知异常");
        updateOnlineWindow(false);
    }
}

void PodMonitor::pollPtz() {
    try {
        auto cap = pod_->getCapabilityAs<IPtzCapability>(CapabilityType::PTZ);
        if (!cap) return;

        auto result = cap->getPose();
        if (result.isSuccess() && result.data.has_value()) {
            std::lock_guard<std::mutex> lk(status_mutex_);
            runtime_status_.ptz_pose       = result.data.value();
            runtime_status_.ptz_update_ms  = nowMs();
            runtime_status_.last_update_ms = runtime_status_.ptz_update_ms;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("[PodMonitor] 云台轮询异常: {}", e.what());
    } catch (...) {
        MYLOG_ERROR("[PodMonitor] 云台轮询未知异常");
    }
}

void PodMonitor::pollLaser() {
    try {
        auto cap = pod_->getCapabilityAs<ILaserCapability>(CapabilityType::LASER);
        if (!cap) return;

        auto result = cap->measure();
        if (result.isSuccess() && result.data.has_value()) {
            std::lock_guard<std::mutex> lk(status_mutex_);
            runtime_status_.laser_info      = result.data.value();
            runtime_status_.laser_update_ms = nowMs();
            runtime_status_.last_update_ms  = runtime_status_.laser_update_ms;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("[PodMonitor] 激光轮询异常: {}", e.what());
    } catch (...) {
        MYLOG_ERROR("[PodMonitor] 激光轮询未知异常");
    }
}

void PodMonitor::pollStream() {
    try {
        auto cap = pod_->getCapabilityAs<IStreamCapability>(CapabilityType::STREAM);
        if (!cap) return;

        auto result = cap->getStreamInfo();
        if (result.isSuccess() && result.data.has_value()) {
            std::lock_guard<std::mutex> lk(status_mutex_);
            runtime_status_.stream_info      = result.data.value();
            runtime_status_.stream_update_ms = nowMs();
            runtime_status_.last_update_ms   = runtime_status_.stream_update_ms;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("[PodMonitor] 流媒体轮询异常: {}", e.what());
    } catch (...) {
        MYLOG_ERROR("[PodMonitor] 流媒体轮询未知异常");
    }
}

// ==================== 在线判定滑动窗口 ====================

void PodMonitor::updateOnlineWindow(bool ping_result) {
    // 先读配置（避免嵌套锁）
    uint32_t window_size;
    uint32_t threshold;
    {
        std::lock_guard<std::mutex> lk(config_mutex_);
        window_size = config_.online_window_size;
        threshold   = config_.online_threshold;
    }

    // 更新窗口和在线状态
    std::lock_guard<std::mutex> lk(status_mutex_);
    online_history_.push_back(ping_result);
    while (online_history_.size() > window_size) {
        online_history_.pop_front();
    }

    uint32_t success_count = 0;
    for (bool ok : online_history_) {
        if (ok) ++success_count;
    }
    runtime_status_.is_online = (success_count >= threshold);
}

} // namespace PodModule
