#pragma once

/**
 * @file pinling_pod.h
 * @brief 品凌吊舱实现
 *
 * 继承 BasePod，覆盖状态查询（Ping 探测）、PTZ 控制（ViewLink SDK）
 * 以及其他品凌特有的能力方法。
 */

#include "../base/base_pod.h"
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>

namespace viewlink { class ViewLinkClient; }

namespace PodModule {

using TelemetryCallback = std::function<void(const PTZPose&)>;

class PinlingPod : public BasePod {
public:
    explicit PinlingPod(const PodInfo& info);
    ~PinlingPod() override;

    // ==================== 连接管理 ====================

    PodResult<void> connect() override;
    PodResult<void> disconnect() override;

    // ==================== 状态查询（Ping 探测） ====================

    PodResult<PodStatus> getDeviceStatus() override;
    PodResult<void> refreshDeviceStatus() override;

    // ==================== 心跳检测 ====================

    PodResult<bool> sendHeartbeat() override;

    // ==================== 云台控制（ViewLink SDK） ====================

    PodResult<PTZPose> getPose() override;
    PodResult<void> setPose(const PTZPose& pose) override;
    PodResult<void> controlSpeed(const PTZCommand& cmd) override;
    PodResult<void> stopPtz() override;
    PodResult<void> goHome() override;

    // ==================== 激光测距 ====================

    PodResult<LaserInfo> laserMeasure() override;
    PodResult<void> enableLaser() override;
    PodResult<void> disableLaser() override;

    // ==================== 流媒体 ====================

    PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) override;
    PodResult<void> stopStream() override;

    // ==================== 图像抓拍 ====================

    PodResult<ImageFrame> captureImage() override;
    PodResult<std::string> saveImage(const std::string& path) override;

    // ==================== 中心点测量 ====================

    PodResult<CenterMeasurementResult> centerMeasure() override;

    // ==================== SDK 扩展（品凌特有） ====================

    PodResult<void> connectFromConfig();
    PodResult<void> connectSDK(const std::string& ip, uint16_t port);
    void disconnectSDK();
    bool isSdkConnected() const;

    PodResult<void> zoomTo(float magnification);
    PodResult<void> zoomSpeed(int speed);
    PodResult<void> stopZoom();
    void setTelemetryCallback(TelemetryCallback cb);

private:
    std::unique_ptr<viewlink::ViewLinkClient> vlk_client_;
    std::atomic<bool> sdk_connected_{false};
    TelemetryCallback telemetry_cb_;
    std::mutex telemetry_mutex_;
};

} // namespace PodModule
