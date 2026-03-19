/**
 * @file dji_pod.cpp
 * @brief 大疆吊舱实现
 */

#include "dji_pod.h"
#include "../../session/dji/dji_session.h"
#include "../../capability/dji/dji_status_capability.h"
#include "../../capability/dji/dji_heartbeat_capability.h"
#include "../../capability/dji/dji_ptz_capability.h"
#include "../../capability/dji/dji_laser_capability.h"
#include "../../capability/dji/dji_stream_capability.h"
#include "../../capability/dji/dji_image_capability.h"
#include "../../capability/dji/dji_center_measure_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiPod::DjiPod(const PodInfo& info) : BasePod(info) {
    MYLOG_INFO("[大疆吊舱] DjiPod 构造: {}", info.pod_id);
    // 初始化大疆 Session
    auto session = std::make_shared<DjiSession>();
    setSession(session);
}

DjiPod::~DjiPod() {
    MYLOG_INFO("[大疆吊舱] DjiPod 析构: {}", pod_info_.pod_id);
}

PodResult<void> DjiPod::initializeCapabilities() {
    MYLOG_INFO("[大疆吊舱] {} 开始装配大疆能力", pod_info_.pod_id);

    // 注册状态查询能力
    addCapability(CapabilityType::STATUS,
                  std::make_shared<DjiStatusCapability>());

    // 注册心跳检测能力
    addCapability(CapabilityType::HEARTBEAT,
                  std::make_shared<DjiHeartbeatCapability>());

    // 注册云台控制能力
    addCapability(CapabilityType::PTZ,
                  std::make_shared<DjiPtzCapability>());

    // 注册激光测距能力
    addCapability(CapabilityType::LASER,
                  std::make_shared<DjiLaserCapability>());

    // 注册流媒体能力
    addCapability(CapabilityType::STREAM,
                  std::make_shared<DjiStreamCapability>());

    // 注册图像抓拍能力
    addCapability(CapabilityType::IMAGE,
                  std::make_shared<DjiImageCapability>());

    // 注册中心点测量能力
    addCapability(CapabilityType::CENTER_MEASURE,
                  std::make_shared<DjiCenterMeasureCapability>());

    MYLOG_INFO("[大疆吊舱] {} 能力装配完成，共 {} 个能力",
               pod_info_.pod_id, capability_registry_.size());
    return PodResult<void>::success("大疆能力装配完成");
}

} // namespace PodModule
