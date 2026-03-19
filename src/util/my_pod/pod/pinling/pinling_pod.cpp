/**
 * @file pinling_pod.cpp
 * @brief 品凌吊舱实现
 */

#include "pinling_pod.h"
#include "../../session/pinling/pinling_session.h"
#include "../../capability/pinling/pinling_status_capability.h"
#include "../../capability/pinling/pinling_heartbeat_capability.h"
#include "../../capability/pinling/pinling_ptz_capability.h"
#include "../../capability/pinling/pinling_laser_capability.h"
#include "../../capability/pinling/pinling_stream_capability.h"
#include "../../capability/pinling/pinling_image_capability.h"
#include "../../capability/pinling/pinling_center_measure_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingPod::PinlingPod(const PodInfo& info) : BasePod(info) {
    MYLOG_INFO("[品凌吊舱] PinlingPod 构造: {}", info.pod_id);
    // 初始化品凌 Session
    auto session = std::make_shared<PinlingSession>();
    setSession(session);
}

PinlingPod::~PinlingPod() {
    MYLOG_INFO("[品凌吊舱] PinlingPod 析构: {}", pod_info_.pod_id);
}

PodResult<void> PinlingPod::initializeCapabilities() {
    MYLOG_INFO("[品凌吊舱] {} 开始装配品凌能力", pod_info_.pod_id);

    // 注册状态查询能力
    addCapability(CapabilityType::STATUS,
                  std::make_shared<PinlingStatusCapability>());

    // 注册心跳检测能力
    addCapability(CapabilityType::HEARTBEAT,
                  std::make_shared<PinlingHeartbeatCapability>());

    // 注册云台控制能力
    addCapability(CapabilityType::PTZ,
                  std::make_shared<PinlingPtzCapability>());

    // 注册激光测距能力
    addCapability(CapabilityType::LASER,
                  std::make_shared<PinlingLaserCapability>());

    // 注册流媒体能力
    addCapability(CapabilityType::STREAM,
                  std::make_shared<PinlingStreamCapability>());

    // 注册图像抓拍能力
    addCapability(CapabilityType::IMAGE,
                  std::make_shared<PinlingImageCapability>());

    // 注册中心点测量能力
    addCapability(CapabilityType::CENTER_MEASURE,
                  std::make_shared<PinlingCenterMeasureCapability>());

    MYLOG_INFO("[品凌吊舱] {} 能力装配完成，共 {} 个能力",
               pod_info_.pod_id, capability_registry_.size());
    return PodResult<void>::success("品凌能力装配完成");
}

} // namespace PodModule
