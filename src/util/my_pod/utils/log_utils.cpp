/**
 * @file log_utils.cpp
 * @brief 吊舱模块日志工具实现
 */

#include "log_utils.h"

namespace PodModule {
namespace LogUtils {

std::string makePodLogPrefix(const std::string& pod_id, const std::string& module) {
    return "[吊舱][" + pod_id + "][" + module + "]";
}

std::string makeCapabilityLogPrefix(const std::string& pod_id, const std::string& capability_name) {
    return "[吊舱][" + pod_id + "][能力:" + capability_name + "]";
}

} // namespace LogUtils
} // namespace PodModule
