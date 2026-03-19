/**
 * @file time_utils.cpp
 * @brief 吊舱模块时间工具实现
 */

#include "time_utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace PodModule {
namespace TimeUtils {

uint64_t currentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint64_t currentTimeMicros() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

std::string timestampToString(uint64_t timestamp_ms) {
    auto seconds = static_cast<time_t>(timestamp_ms / 1000);
    auto ms = timestamp_ms % 1000;
    std::tm tm_buf{};
    localtime_r(&seconds, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

} // namespace TimeUtils
} // namespace PodModule
