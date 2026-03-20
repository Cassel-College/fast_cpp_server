#pragma once

/**
 * @file capability_types.h
 * @brief 能力类型枚举定义
 * 
 * 定义吊舱支持的所有能力类型标识。
 */

#include <string>
#include <cstdint>

namespace PodModule {

/**
 * @brief 能力类型枚举
 * 
 * 用于标识吊舱可注册的各种能力。
 */
enum class CapabilityType : uint16_t {
    UNKNOWN          = 0,    // 未知能力
    STATUS           = 1,    // 状态查询能力
    HEARTBEAT        = 2,    // 心跳检测能力
    PTZ              = 3,    // 云台控制能力（Pan-Tilt-Zoom）
    LASER            = 4,    // 激光测距能力
    STREAM           = 5,    // 流媒体能力
    IMAGE            = 6,    // 图像抓拍能力
    CENTER_MEASURE   = 7,    // 中心点测量能力
};

/**
 * @brief 将能力类型枚举转为中文字符串
 */
std::string capabilityTypeToString(CapabilityType type);

/**
 * @brief 将能力类型枚举转为稳定英文标识
 */
std::string capabilityTypeToKey(CapabilityType type);

/**
 * @brief 从字符串解析能力类型
 * @param value 支持英文标识、枚举名和中文名称
 * @param type 输出能力类型
 * @return 解析成功返回 true
 */
bool capabilityTypeFromString(const std::string& value, CapabilityType& type);

} // namespace PodModule
