#pragma once

/**
 * @file time_utils.h
 * @brief 吊舱模块时间工具
 */

#include <cstdint>
#include <string>

namespace PodModule {
namespace TimeUtils {

/** @brief 获取当前时间戳（毫秒） */
uint64_t currentTimeMillis();

/** @brief 获取当前时间戳（微秒） */
uint64_t currentTimeMicros();

/** @brief 将时间戳转为可读字符串 */
std::string timestampToString(uint64_t timestamp_ms);

} // namespace TimeUtils
} // namespace PodModule
