#pragma once

/**
 * @file pod_types.h
 * @brief 吊舱模块基础类型定义
 * 
 * 定义吊舱厂商枚举、设备状态枚举、流类型枚举等基础类型。
 */

#include <string>
#include <cstdint>

namespace PodModule {

/**
 * @brief 吊舱厂商枚举
 */
enum class PodVendor : uint8_t {
    UNKNOWN = 0,    // 未知厂商
    DJI     = 1,    // 大疆
    PINLING = 2,    // 品凌
};

/**
 * @brief 吊舱设备状态枚举
 */
enum class PodState : uint8_t {
    DISCONNECTED = 0,   // 已断开
    CONNECTING   = 1,   // 连接中
    CONNECTED    = 2,   // 已连接
    ERROR        = 3,   // 异常
};

/**
 * @brief 流类型枚举
 */
enum class StreamType : uint8_t {
    UNKNOWN = 0,    // 未知
    RTSP    = 1,    // RTSP流
    RTMP    = 2,    // RTMP流
    HLS     = 3,    // HLS流
    RAW     = 4,    // 原始流
};

/**
 * @brief 将厂商枚举转为中文字符串
 */
std::string podVendorToString(PodVendor vendor);

/**
 * @brief 将设备状态枚举转为中文字符串
 */
std::string podStateToString(PodState state);

/**
 * @brief 将流类型枚举转为字符串
 */
std::string streamTypeToString(StreamType type);

} // namespace PodModule
