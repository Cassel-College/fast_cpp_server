#pragma once

#include <string>

namespace my_tools::ping_tools {

class PingFuncByCPR {
public:
    static bool PingURL(const std::string& url);
};

class PingFuncBySystem {
public:
    static bool PingIP(const std::string& ip, int count = 1, int timeout_sec = 1);

    static bool PingIPBySocket(const std::string& ip, int port = 1, int timeout_sec = 1);
};

} // namespace my_tools::ping_tools