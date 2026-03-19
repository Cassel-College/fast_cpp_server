/**
 * @file string_utils.cpp
 * @brief 吊舱模块字符串工具实现
 */

#include "string_utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace PodModule {
namespace StringUtils {

std::string format(const std::string& fmt, const std::string& arg) {
    std::string result = fmt;
    auto pos = result.find("{}");
    if (pos != std::string::npos) {
        result.replace(pos, 2, arg);
    }
    return result;
}

std::string bytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (size_t i = 0; i < data.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
        if (i + 1 < data.size()) {
            oss << " ";
        }
    }
    return oss.str();
}

std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(),
                                   [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(),
                                 [](unsigned char c) { return std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

} // namespace StringUtils
} // namespace PodModule
