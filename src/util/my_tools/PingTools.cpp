#include "PingTools.h"

#include <cpr/cpr.h>

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

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

bool PingFuncBySystem::PingIPBySocket(const std::string& ip, int port, int timeout_sec) {
    if (ip.empty()) {
        MYLOG_WARN("Socket ping 失败：ip 不能为空");
        return false;
    }

    if (port <= 0 || port > 65535 || timeout_sec <= 0) {
        MYLOG_WARN("Socket ping 参数非法: ip={}, port={}, timeout_sec={}", ip, port, timeout_sec);
        return false;
    }

    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        MYLOG_ERROR("Socket ping 创建 socket 失败: {}", std::strerror(errno));
        return false;
    }

    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        MYLOG_WARN("Socket ping 无效 IP 地址: {}", ip);
        ::close(sock);
        return false;
    }

    const int ret = ::connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    const int saved_errno = errno;
    ::close(sock);

    if (ret == 0) {
        MYLOG_INFO("Socket ping {}:{} -> reachable=true", ip, port);
        return true;
    }

    MYLOG_INFO("Socket ping {}:{} -> reachable=false, error={}", ip, port, std::strerror(saved_errno));
    return false;
}

} // namespace my_tools::ping_tools