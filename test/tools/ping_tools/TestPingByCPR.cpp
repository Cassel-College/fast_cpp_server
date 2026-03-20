#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <thread>

#include "PingTools.h"

namespace {

class LocalHttpServer {
public:
    explicit LocalHttpServer(int status_code)
        : status_code_(status_code) {
        server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            throw std::runtime_error("failed to create socket");
        }

        int opt = 1;
        ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            cleanup();
            throw std::runtime_error("failed to bind socket");
        }

        if (::listen(server_fd_, 1) < 0) {
            cleanup();
            throw std::runtime_error("failed to listen on socket");
        }

        socklen_t len = sizeof(addr);
        if (::getsockname(server_fd_, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
            cleanup();
            throw std::runtime_error("failed to get socket name");
        }

        port_ = ntohs(addr.sin_port);
        thread_ = std::thread([this]() { serveOnce(); });
    }

    ~LocalHttpServer() {
        cleanup();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string url() const {
        return "http://127.0.0.1:" + std::to_string(port_);
    }

private:
    void serveOnce() const {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = ::accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            return;
        }

        char request_buffer[1024];
        ::recv(client_fd, request_buffer, sizeof(request_buffer), 0);

        const std::string reason = status_code_ == 200 ? "OK" : "NOT FOUND";
        const std::string response =
            "HTTP/1.1 " + std::to_string(status_code_) + " " + reason + "\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";

        ::send(client_fd, response.c_str(), response.size(), 0);
        ::shutdown(client_fd, SHUT_RDWR);
        ::close(client_fd);
    }

    void cleanup() {
        if (server_fd_ >= 0) {
            ::shutdown(server_fd_, SHUT_RDWR);
            ::close(server_fd_);
            server_fd_ = -1;
        }
    }

private:
    int status_code_ = 200;
    int server_fd_ = -1;
    int port_ = 0;
    std::thread thread_;
};

std::string configuredPodUrl() {
    return "http://192.168.2.119";
}

} // namespace

TEST(PingByCPRTest, ReturnsTrueForHttp200) {
    LocalHttpServer server(200);
    const bool result = my_tools::ping_tools::PingFuncByCPR::PingURL(server.url());
    EXPECT_TRUE(result);
}

TEST(PingByCPRTest, ReturnsFalseForHttp404) {
    LocalHttpServer server(404);
    const bool result = my_tools::ping_tools::PingFuncByCPR::PingURL(server.url());
    EXPECT_FALSE(result);
}

TEST(PingByCPRTest, PingConfiguredPodIpIfReachable) {
    const std::string url = configuredPodUrl();
    const bool result = my_tools::ping_tools::PingFuncByCPR::PingURL(url);
    if (!result) {
        GTEST_SKIP() << "configured pod endpoint is unreachable or did not return HTTP 200: " << url;
    }
    SUCCEED();
}

TEST(PingBySystemTest, ReturnsTrueForLoopbackIp) {
    const bool result = my_tools::ping_tools::PingFuncBySystem::PingIP("127.0.0.1");
    EXPECT_TRUE(result);
}

TEST(PingBySystemTest, ReturnsFalseForUnreachableDocumentationIp) {
    const bool result = my_tools::ping_tools::PingFuncBySystem::PingIP("203.0.113.1");
    EXPECT_FALSE(result);
}