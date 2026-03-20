/**
 * @file capability_types.cpp
 * @brief 能力类型枚举实现
 */

#include "capability_types.h"

#include <cctype>

namespace {

std::string normalizeCapabilityText(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        if (std::isspace(uch) || ch == '_' || ch == '-') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(uch)));
    }

    return normalized;
}

} // namespace

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

std::string capabilityTypeToKey(CapabilityType type) {
    switch (type) {
        case CapabilityType::STATUS:         return "STATUS";
        case CapabilityType::HEARTBEAT:      return "HEARTBEAT";
        case CapabilityType::PTZ:            return "PTZ";
        case CapabilityType::LASER:          return "LASER";
        case CapabilityType::STREAM:         return "STREAM";
        case CapabilityType::IMAGE:          return "IMAGE";
        case CapabilityType::CENTER_MEASURE: return "CENTER_MEASURE";
        default:                             return "UNKNOWN";
    }
}

bool capabilityTypeFromString(const std::string& value, CapabilityType& type) {
    const std::string normalized = normalizeCapabilityText(value);

    if (normalized.empty()) {
        return false;
    }

    if (normalized == "status" || normalized == "状态查询") {
        type = CapabilityType::STATUS;
        return true;
    }
    if (normalized == "heartbeat" || normalized == "心跳检测") {
        type = CapabilityType::HEARTBEAT;
        return true;
    }
    if (normalized == "ptz" || normalized == "云台控制") {
        type = CapabilityType::PTZ;
        return true;
    }
    if (normalized == "laser" || normalized == "激光测距") {
        type = CapabilityType::LASER;
        return true;
    }
    if (normalized == "stream" || normalized == "流媒体") {
        type = CapabilityType::STREAM;
        return true;
    }
    if (normalized == "image" || normalized == "图像抓拍") {
        type = CapabilityType::IMAGE;
        return true;
    }
    if (normalized == "centermeasure" || normalized == "中心点测量") {
        type = CapabilityType::CENTER_MEASURE;
        return true;
    }

    return false;
}

} // namespace PodModule
