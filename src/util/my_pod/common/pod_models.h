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
    std::string pod_id;                         // 吊舱唯一标识
    std::string pod_name;                       // 吊舱名称
    PodVendor   vendor = PodVendor::UNKNOWN;    // 厂商
    std::string model;                          // 型号
    std::string firmware_ver;                   // 固件版本
    std::string ip_address;                     // IP地址
    uint16_t    port = 0;                       // 端口号
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

/**
 * @brief 吊舱运行时状态聚合数据
 *
 * 由 PodMonitor 后台线程定期采集各能力模块数据后更新，
 * 外部读取时直接返回此缓存快照，无需实时查询设备。
 */
struct PodRuntimeStatus {
    // ---- 在线状态（滑动窗口判定） ----
    bool        is_online = false;              // 综合判定：设备是否在线

    // ---- 设备状态 ----
    PodStatus   pod_status;                     // 状态能力采集结果

    // ---- 云台姿态 ----
    PTZPose     ptz_pose;                       // 云台姿态采集结果

    // ---- 激光测距 ----
    LaserInfo   laser_info;                     // 激光能力采集结果

    // ---- 流媒体 ----
    StreamInfo  stream_info;                    // 流媒体能力采集结果

    // ---- 时间戳（毫秒） ----
    uint64_t    last_update_ms       = 0;       // 最近一次任意数据更新时间
    uint64_t    online_update_ms     = 0;       // 在线状态更新时间
    uint64_t    ptz_update_ms        = 0;       // 云台数据更新时间
    uint64_t    laser_update_ms      = 0;       // 激光数据更新时间
    uint64_t    stream_update_ms     = 0;       // 流媒体数据更新时间
};

/**
 * @brief PodMonitor 轮询配置
 *
 * 控制后台监控线程的轮询行为：各能力的轮询开关与间隔、
 * 在线判定滑动窗口参数、主循环睡眠间隔等。
 */
struct PodMonitorConfig {
    uint32_t poll_interval_ms       = 1000;     // 主循环迭代间隔（毫秒）
    uint32_t status_interval_ms     = 5000;     // 在线/状态检测间隔
    uint32_t ptz_interval_ms        = 1000;     // 云台姿态轮询间隔
    uint32_t laser_interval_ms      = 5000;     // 激光数据轮询间隔
    uint32_t stream_interval_ms     = 5000;     // 流媒体状态轮询间隔

    uint32_t online_window_size     = 3;        // 在线判定滑动窗口大小
    uint32_t online_threshold       = 2;        // 窗口中>=此数为在线

    bool enable_status_poll         = true;     // 是否轮询状态/在线检测
    bool enable_ptz_poll            = true;     // 是否轮询云台姿态
    bool enable_laser_poll          = false;    // 是否轮询激光（默认关闭）
    bool enable_stream_poll         = true;     // 是否轮询流媒体状态

    bool auto_start                 = true;     // 创建 Pod 后是否自动启动监控
};

} // namespace PodModule
