#pragma once

/**
 * @file log_utils.h
 * @brief 吊舱模块日志工具
 * 
 * 封装吊舱模块专用的日志辅助函数。
 */

#include <string>

namespace PodModule {
namespace LogUtils {

/** @brief 生成统一格式的吊舱日志前缀 */
std::string makePodLogPrefix(const std::string& pod_id, const std::string& module);

/** @brief 生成统一格式的能力日志前缀 */
std::string makeCapabilityLogPrefix(const std::string& pod_id, const std::string& capability_name);

} // namespace LogUtils
} // namespace PodModule
