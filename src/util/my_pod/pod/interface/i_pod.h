#pragma once

/**
 * @file i_pod.h
 * @brief 吊舱统一抽象接口
 *
 * 定义所有吊舱设备的统一抽象接口。
 * 所有能力方法（状态、心跳、PTZ、激光、流媒体、图像、中心测量）
 * 直接作为 Pod 的虚方法，由 BasePod 提供默认占位实现，
 * 厂商 Pod（DjiPod、PinlingPod）按需覆盖。
 */

#include "../../common/pod_types.h"
#include "../../common/pod_models.h"
#include "../../common/pod_result.h"
#include "../../common/capability_types.h"
#include "../../session/interface/i_session.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace PodModule {

class IPod {
public:
    virtual ~IPod() = default;

    // ==================== 设备基础信息 ====================

    virtual PodInfo getPodInfo() const = 0;
    virtual std::string getPodId() const = 0;
    virtual std::string getPodName() const = 0;
    virtual PodVendor getVendor() const = 0;

    // ==================== 连接管理 ====================

    virtual PodResult<void> connect() = 0;
    virtual PodResult<void> disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual PodState getState() const = 0;

    // ==================== Session 管理 ====================

    virtual void setSession(std::shared_ptr<ISession> session) = 0;
    virtual std::shared_ptr<ISession> getSession() const = 0;

    // ==================== 配置 ====================

    virtual void setCapabilityConfigs(const nlohmann::json& capability_section) = 0;
    virtual nlohmann::json getCapabilityConfig(CapabilityType type) const = 0;

    // ==================== 后台监控 ====================

    virtual PodRuntimeStatus getRuntimeStatus() const = 0;
    virtual void startMonitor(const PodMonitorConfig& config = {}) = 0;
    virtual void stopMonitor() = 0;
    virtual bool isMonitorRunning() const = 0;

    // ==================== 状态查询 ====================

    virtual PodResult<PodStatus> getDeviceStatus() = 0;
    virtual PodResult<void> refreshDeviceStatus() = 0;

    // ==================== 心跳检测 ====================

    virtual PodResult<bool> sendHeartbeat() = 0;
    virtual PodResult<void> startHeartbeat(uint32_t interval_ms = 1000) = 0;
    virtual PodResult<void> stopHeartbeat() = 0;
    virtual bool isAlive() const = 0;

    // ==================== 云台控制（PTZ） ====================

    virtual PodResult<PTZPose> getPose() = 0;
    virtual PodResult<void> setPose(const PTZPose& pose) = 0;
    virtual PodResult<void> controlSpeed(const PTZCommand& cmd) = 0;
    virtual PodResult<void> stopPtz() = 0;
    virtual PodResult<void> goHome() = 0;

    // ==================== 激光测距 ====================

    virtual PodResult<LaserInfo> laserMeasure() = 0;
    virtual PodResult<void> enableLaser() = 0;
    virtual PodResult<void> disableLaser() = 0;
    virtual bool isLaserEnabled() const = 0;

    // ==================== 流媒体 ====================

    virtual PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) = 0;
    virtual PodResult<void> stopStream() = 0;
    virtual PodResult<StreamInfo> getStreamInfo() = 0;
    virtual bool isStreaming() const = 0;

    // ==================== 图像抓拍 ====================

    virtual PodResult<ImageFrame> captureImage() = 0;
    virtual PodResult<std::string> saveImage(const std::string& path) = 0;

    // ==================== 中心点测量 ====================

    virtual PodResult<CenterMeasurementResult> centerMeasure() = 0;
    virtual PodResult<void> startContinuousMeasure(uint32_t interval_ms = 500) = 0;
    virtual PodResult<void> stopContinuousMeasure() = 0;
    virtual PodResult<CenterMeasurementResult> getLastMeasureResult() = 0;
};

} // namespace PodModule
