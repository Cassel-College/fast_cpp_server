/**
 * @file pod_types.cpp
 * @brief 吊舱模块基础类型实现
 */

#include "pod_types.h"

namespace PodModule {

std::string podVendorToString(PodVendor vendor) {
    switch (vendor) {
        case PodVendor::DJI:     return "大疆";
        case PodVendor::PINLING: return "品凌";
        default:                 return "未知厂商";
    }
}

std::string podStateToString(PodState state) {
    switch (state) {
        case PodState::DISCONNECTED: return "已断开";
        case PodState::CONNECTING:   return "连接中";
        case PodState::CONNECTED:    return "已连接";
        case PodState::ERROR:        return "异常";
        default:                     return "未知状态";
    }
}

std::string streamTypeToString(StreamType type) {
    switch (type) {
        case StreamType::RTSP: return "RTSP";
        case StreamType::RTMP: return "RTMP";
        case StreamType::HLS:  return "HLS";
        case StreamType::RAW:  return "RAW";
        default:               return "未知";
    }
}

} // namespace PodModule
