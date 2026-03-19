#pragma once

/**
 * @file pod_errors.h
 * @brief 吊舱模块错误码定义
 */

#include <string>
#include <cstdint>

namespace PodModule {

/**
 * @brief 吊舱错误码枚举
 */
enum class PodErrorCode : int32_t {
    SUCCESS                 = 0,        // 成功
    UNKNOWN_ERROR           = -1,       // 未知错误
    CONNECTION_FAILED       = -100,     // 连接失败
    CONNECTION_TIMEOUT      = -101,     // 连接超时
    ALREADY_CONNECTED       = -102,     // 已连接
    NOT_CONNECTED           = -103,     // 未连接
    DISCONNECT_FAILED       = -104,     // 断开失败
    SESSION_ERROR           = -200,     // 会话错误
    SESSION_NOT_INITIALIZED = -201,     // 会话未初始化
    SEND_FAILED             = -202,     // 发送失败
    RECEIVE_FAILED          = -203,     // 接收失败
    CAPABILITY_NOT_FOUND    = -300,     // 能力未找到
    CAPABILITY_NOT_SUPPORTED= -301,     // 能力不支持
    CAPABILITY_INIT_FAILED  = -302,     // 能力初始化失败
    PTZ_CONTROL_FAILED      = -400,     // 云台控制失败
    LASER_MEASURE_FAILED    = -500,     // 激光测距失败
    STREAM_START_FAILED     = -600,     // 拉流启动失败
    STREAM_STOP_FAILED      = -601,     // 拉流停止失败
    IMAGE_CAPTURE_FAILED    = -700,     // 图像抓拍失败
    CENTER_MEASURE_FAILED   = -800,     // 中心测量失败
    POD_NOT_FOUND           = -900,     // 吊舱未找到
    POD_ALREADY_EXISTS      = -901,     // 吊舱已存在
};

/**
 * @brief 将错误码转为中文描述
 */
std::string podErrorCodeToString(PodErrorCode code);

} // namespace PodModule
