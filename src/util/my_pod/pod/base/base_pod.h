#pragma once

/**
 * @file base_pod.h
 * @brief 基础吊舱实现
 *
 * BasePod 是所有厂商 Pod 的基类：
 * - 保存设备元信息（PodInfo）
 * - 保存当前设备状态（PodState）
 * - 持有 Session 会话对象
 * - 提供基础 connect / disconnect 逻辑
 * - 为所有能力方法提供默认占位实现
 * - 厂商 Pod 按需覆盖具体能力方法
 */

#include "../interface/i_pod.h"
#include "../../session/interface/i_session.h"
#include "../../monitor/pod_monitor.h"
#include <memory>
#include <atomic>

namespace PodModule {

class BasePod : public IPod {
public:
    explicit BasePod(const PodInfo& info);
    ~BasePod() override;

    // ==================== 设备基础信息 ====================

    PodInfo getPodInfo() const override;
    std::string getPodId() const override;
    std::string getPodName() const override;
    PodVendor getVendor() const override;

    // ==================== 连接管理 ====================

    PodResult<void> connect() override;
    PodResult<void> disconnect() override;
    bool isConnected() const override;
    PodState getState() const override;

    // ==================== Session 管理 ====================

    void setSession(std::shared_ptr<ISession> session) override;
    std::shared_ptr<ISession> getSession() const override;

    // ==================== 配置 ====================

    void setCapabilityConfigs(const nlohmann::json& capability_section) override;
    nlohmann::json getCapabilityConfig(CapabilityType type) const override;

    // ==================== 后台监控 ====================

    PodRuntimeStatus getRuntimeStatus() const override;
    void startMonitor(const PodMonitorConfig& config = {}) override;
    void stopMonitor() override;
    bool isMonitorRunning() const override;

    // ==================== 状态查询（默认占位） ====================

    PodResult<PodStatus> getDeviceStatus() override;
    PodResult<void> refreshDeviceStatus() override;

    // ==================== 心跳检测（默认占位） ====================

    PodResult<bool> sendHeartbeat() override;
    PodResult<void> startHeartbeat(uint32_t interval_ms = 1000) override;
    PodResult<void> stopHeartbeat() override;
    bool isAlive() const override;

    // ==================== 云台控制（默认占位） ====================

    PodResult<PTZPose> getPose() override;
    PodResult<void> setPose(const PTZPose& pose) override;
    PodResult<void> controlSpeed(const PTZCommand& cmd) override;
    PodResult<void> stopPtz() override;
    PodResult<void> goHome() override;

    // ==================== 激光测距（默认占位） ====================

    PodResult<LaserInfo> laserMeasure() override;
    PodResult<void> enableLaser() override;
    PodResult<void> disableLaser() override;
    bool isLaserEnabled() const override;

    // ==================== 流媒体（默认占位） ====================

    PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) override;
    PodResult<void> stopStream() override;
    PodResult<StreamInfo> getStreamInfo() override;
    bool isStreaming() const override;

    // ==================== 图像抓拍（默认占位） ====================

    PodResult<ImageFrame> captureImage() override;
    PodResult<std::string> saveImage(const std::string& path) override;

    // ==================== 中心点测量（默认占位） ====================

    PodResult<CenterMeasurementResult> centerMeasure() override;
    PodResult<void> startContinuousMeasure(uint32_t interval_ms = 500) override;
    PodResult<void> stopContinuousMeasure() override;
    PodResult<CenterMeasurementResult> getLastMeasureResult() override;

protected:
    void setState(PodState state);

    PodInfo             pod_info_;
    PodState            state_ = PodState::DISCONNECTED;
    std::shared_ptr<ISession> session_;
    PodMonitor          monitor_;
    nlohmann::json      capability_configs_;

    // 能力相关缓存数据（子类共享）
    PodStatus           cached_status_;
    std::atomic<bool>   alive_{false};
    std::atomic<bool>   heartbeat_running_{false};
    uint32_t            heartbeat_interval_ms_ = 1000;
    PTZPose             current_pose_;
    std::atomic<bool>   laser_enabled_{false};
    LaserInfo           last_laser_measurement_;
    std::atomic<bool>   streaming_{false};
    StreamInfo          current_stream_;
    CenterMeasurementResult last_center_result_;
    std::atomic<bool>   continuous_measure_running_{false};
    uint32_t            measure_interval_ms_ = 500;
};

} // namespace PodModule
