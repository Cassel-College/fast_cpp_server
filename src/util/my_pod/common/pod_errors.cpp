/**
 * @file pod_errors.cpp
 * @brief 吊舱模块错误码实现
 */

#include "pod_errors.h"

namespace PodModule {

std::string podErrorCodeToString(PodErrorCode code) {
    switch (code) {
        case PodErrorCode::SUCCESS:                  return "成功";
        case PodErrorCode::UNKNOWN_ERROR:            return "未知错误";
        case PodErrorCode::CONNECTION_FAILED:        return "连接失败";
        case PodErrorCode::CONNECTION_TIMEOUT:       return "连接超时";
        case PodErrorCode::ALREADY_CONNECTED:        return "已连接";
        case PodErrorCode::NOT_CONNECTED:            return "未连接";
        case PodErrorCode::DISCONNECT_FAILED:        return "断开失败";
        case PodErrorCode::SESSION_ERROR:            return "会话错误";
        case PodErrorCode::SESSION_NOT_INITIALIZED:  return "会话未初始化";
        case PodErrorCode::SEND_FAILED:              return "发送失败";
        case PodErrorCode::RECEIVE_FAILED:           return "接收失败";
        case PodErrorCode::CAPABILITY_NOT_FOUND:     return "能力未找到";
        case PodErrorCode::CAPABILITY_NOT_SUPPORTED: return "能力不支持";
        case PodErrorCode::CAPABILITY_INIT_FAILED:   return "能力初始化失败";
        case PodErrorCode::PTZ_CONTROL_FAILED:       return "云台控制失败";
        case PodErrorCode::LASER_MEASURE_FAILED:     return "激光测距失败";
        case PodErrorCode::STREAM_START_FAILED:      return "拉流启动失败";
        case PodErrorCode::STREAM_STOP_FAILED:       return "拉流停止失败";
        case PodErrorCode::IMAGE_CAPTURE_FAILED:     return "图像抓拍失败";
        case PodErrorCode::CENTER_MEASURE_FAILED:    return "中心测量失败";
        case PodErrorCode::POD_NOT_FOUND:            return "吊舱未找到";
        case PodErrorCode::POD_ALREADY_EXISTS:       return "吊舱已存在";
        default:                                     return "未知错误码";
    }
}

} // namespace PodModule
