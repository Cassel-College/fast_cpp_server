/**
 * @file capability_types.cpp
 * @brief 能力类型枚举实现
 */

#include "capability_types.h"

namespace PodModule {

std::string capabilityTypeToString(CapabilityType type) {
    switch (type) {
        case CapabilityType::STATUS:         return "状态查询";
        case CapabilityType::HEARTBEAT:      return "心跳检测";
        case CapabilityType::PTZ:            return "云台控制";
        case CapabilityType::LASER:          return "激光测距";
        case CapabilityType::STREAM:         return "流媒体";
        case CapabilityType::IMAGE:          return "图像抓拍";
        case CapabilityType::CENTER_MEASURE: return "中心点测量";
        default:                             return "未知能力";
    }
}

} // namespace PodModule
