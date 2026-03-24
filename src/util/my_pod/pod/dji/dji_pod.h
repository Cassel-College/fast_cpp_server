#pragma once

/**
 * @file dji_pod.h
 * @brief 大疆吊舱实现
 *
 * 继承 BasePod，覆盖大疆特有的能力方法。
 * 当前均为占位实现，后续通过 DjiSession 与设备通信。
 */

#include "../base/base_pod.h"

namespace PodModule {

class DjiPod : public BasePod {
public:
    explicit DjiPod(const PodInfo& info);
    ~DjiPod() override;

    // ==================== 状态查询 ====================

    PodResult<PodStatus> getDeviceStatus() override;
    PodResult<void> refreshDeviceStatus() override;

    // ==================== 心跳检测 ====================

    PodResult<bool> sendHeartbeat() override;

    // ==================== 云台控制 ====================

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
};

} // namespace PodModule
