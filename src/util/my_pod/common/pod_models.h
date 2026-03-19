#pragma once

/**
 * @file pod_models.h
 * @brief 吊舱模块数据模型定义
 * 
 * 定义吊舱设备信息、云台姿态、激光信息、流信息、图像帧、
 * 中心测量结果等模型结构体。
 */

#include "pod_types.h"
#include <string>
#include <cstdint>
#include <vector>

namespace PodModule {

/**
 * @brief 吊舱设备基础信息
 */
struct PodInfo {
    std::string pod_id;         // 吊舱唯一标识
    std::string pod_name;       // 吊舱名称
    PodVendor   vendor = PodVendor::UNKNOWN;  // 厂商
    std::string model;          // 型号
    std::string firmware_ver;   // 固件版本
    std::string ip_address;     // IP地址
    uint16_t    port = 0;       // 端口号
};

/**
 * @brief 吊舱实时状态
 */
struct PodStatus {
    PodState    state = PodState::DISCONNECTED; // 设备状态
    double      temperature = 0.0;              // 设备温度（℃）
    double      voltage = 0.0;                  // 供电电压（V）
    uint64_t    uptime_ms = 0;                  // 运行时间（毫秒）
    std::string error_msg;                      // 错误信息
};

/**
 * @brief 云台姿态（PTZ）
 */
struct PTZPose {
    double yaw   = 0.0;    // 偏航角（°）
    double pitch  = 0.0;    // 俯仰角（°）
    double roll   = 0.0;    // 滚转角（°）
    double zoom   = 1.0;    // 变焦倍率
};

/**
 * @brief 云台控制命令
 */
struct PTZCommand {
    double yaw_speed   = 0.0;  // 偏航速度（°/s）
    double pitch_speed = 0.0;  // 俯仰速度（°/s）
    double zoom_speed  = 0.0;  // 变焦速度
};

/**
 * @brief 激光测距信息
 */
struct LaserInfo {
    bool   is_valid   = false;  // 测量是否有效
    double distance   = 0.0;    // 距离（米）
    double latitude   = 0.0;    // 目标纬度
    double longitude  = 0.0;    // 目标经度
    double altitude   = 0.0;    // 目标海拔（米）
};

/**
 * @brief 流媒体信息
 */
struct StreamInfo {
    StreamType  type = StreamType::UNKNOWN;  // 流类型
    std::string url;                          // 流地址
    int         width  = 0;                   // 画面宽度
    int         height = 0;                   // 画面高度
    int         fps    = 0;                   // 帧率
    bool        is_active = false;            // 是否正在推流
};

/**
 * @brief 图像帧数据
 */
struct ImageFrame {
    int                  width  = 0;       // 图像宽度
    int                  height = 0;       // 图像高度
    int                  channels = 3;     // 通道数
    std::vector<uint8_t> data;             // 图像原始数据
    uint64_t             timestamp_ms = 0; // 时间戳（毫秒）
    std::string          format;           // 图像格式（如 "JPEG", "PNG"）
};

/**
 * @brief 中心点测量结果
 * 
 * 测量画面中心位置目标到激光仪的距离及目标坐标。
 */
struct CenterMeasurementResult {
    bool   is_valid   = false;  // 测量是否有效
    double distance   = 0.0;    // 距离（米）
    double latitude   = 0.0;    // 目标纬度
    double longitude  = 0.0;    // 目标经度
    double altitude   = 0.0;    // 目标海拔（米）
    double center_x   = 0.0;    // 画面中心x坐标
    double center_y   = 0.0;    // 画面中心y坐标
};

} // namespace PodModule
