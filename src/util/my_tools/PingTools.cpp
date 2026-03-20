#include "PingTools.h"

#include <cpr/cpr.h>

#include <cstdlib>
#include <sstream>
#include <sys/wait.h>

#include "MyLog.h"

namespace {

std::string quoteForShell(const std::string& value) {
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

bool isExitSuccess(int status) {
    return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace

namespace my_tools::ping_tools {

bool PingFuncByCPR::PingURL(const std::string& url) {
    try {
        auto response = cpr::Get(cpr::Url{url}, cpr::Timeout{3000});
        MYLOG_INFO("Ping {} - status code: {}", url, response.status_code);
        return response.status_code == 200;
    } catch (const std::exception& e) {
        MYLOG_ERROR("Ping {} failed: {}", url, e.what());
        return false;
    }
}

bool PingFuncBySystem::PingIP(const std::string& ip, int count, int timeout_sec) {
    if (ip.empty()) {
        MYLOG_WARN("系统 ping 失败：ip 不能为空");
        return false;
    }

    if (count <= 0 || timeout_sec <= 0) {
        MYLOG_WARN("系统 ping 参数非法: ip={}, count={}, timeout_sec={}", ip, count, timeout_sec);
        return false;
    }

    std::ostringstream command;
    command << "ping -c " << count
            << " -W " << timeout_sec
            << " " << quoteForShell(ip)
            << " > /dev/null 2>&1";

    const int status = std::system(command.str().c_str());
    const bool reachable = isExitSuccess(status);
    MYLOG_INFO("System ping {} -> reachable={}, raw_status={}", ip, reachable, status);
    return reachable;
}

} // namespace my_tools::ping_tools