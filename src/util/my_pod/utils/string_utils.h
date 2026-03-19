#pragma once

/**
 * @file string_utils.h
 * @brief 吊舱模块字符串工具
 */

#include <string>
#include <vector>
#include <cstdint>

namespace PodModule {
namespace StringUtils {

/** @brief 字符串格式化（简易版） */
std::string format(const std::string& fmt, const std::string& arg);

/** @brief 将字节数组转为十六进制字符串 */
std::string bytesToHex(const std::vector<uint8_t>& data);

/** @brief 去除字符串前后空格 */
std::string trim(const std::string& str);

} // namespace StringUtils
} // namespace PodModule
