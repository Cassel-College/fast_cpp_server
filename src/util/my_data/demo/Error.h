#pragma once
#include <string>

namespace my_data {

/**
 * @brief 统一错误码（MVP）
 * @note 后续可根据需要扩展
 * Ok: 操作成功
 * InvalidCommand: 无效命令
 * UnknownDevice: 未知设备
 * DeviceOffline: 设备离线
 * Timeout: 操作超时
 * DriverError: 设备驱动错误
 * EStop: 紧急停止
 * InternalError: 内部错误
 */
enum class ErrorCode {
  Ok = 0,
  InvalidCommand = 1,
  UnknownDevice = 2,
  DeviceOffline = 3,
  Timeout = 4,
  DriverError = 5,
  EStop = 6,
  InternalError = 7,
};

/**
 * @brief 将错误码转为可读字符串
 */
std::string ToString(ErrorCode code);

} // namespace my_data