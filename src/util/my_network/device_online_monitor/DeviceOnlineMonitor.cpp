#include "DeviceOnlineMonitor.h"
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

DeviceOnlineMonitor::DeviceOnlineMonitor()
    : host_(""),
      port_(0),
      interval_seconds_(5),
      fail_threshold_(3),
      running_(false),
      online_(false),
      initialized_(false) {}

DeviceOnlineMonitor::DeviceOnlineMonitor(const std::string& host, int port, int interval_seconds, int fail_threshold)
    : host_(host),
      port_(port),
      interval_seconds_(interval_seconds),
      fail_threshold_(fail_threshold),
      running_(false),
      online_(false),
      initialized_(true) {}

bool DeviceOnlineMonitor::init(const std::string& host, int port, int interval_seconds, int fail_threshold) {
    if (running_) return false;           // 不允许在运行中初始化
    if (initialized_) return false;       // 防止重复初始化

    host_ = host;
    port_ = port;
    interval_seconds_ = interval_seconds;
    fail_threshold_ = fail_threshold;
    initialized_ = true;
    return true;
}

DeviceOnlineMonitor::~DeviceOnlineMonitor() {
    stop();
}

void DeviceOnlineMonitor::start() {
    if (!initialized_ || running_) return;
    running_ = true;
    monitor_thread_ = std::thread(&DeviceOnlineMonitor::monitorLoop, this);
}

void DeviceOnlineMonitor::stop() {
    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

bool DeviceOnlineMonitor::isOnline() {
    std::lock_guard<std::mutex> lock(mutex_);
    return online_;
}

void DeviceOnlineMonitor::monitorLoop() {
    int fail_count = 0;

    while (running_) {
        bool success = tryConnect();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (success) {
                fail_count = 0;
                online_ = true;
            } else {
                fail_count++;
                if (fail_count >= fail_threshold_) {
                    online_ = false;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds_));
    }
}

bool DeviceOnlineMonitor::tryConnect() {

    bool connectStatus = false;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return connectStatus;
    try {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
            close(sock);
            return connectStatus;
        }

        // 设置超时
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (result == 0) {
            connectStatus = true;
        }
    } catch (...) {
        std::string errorMsg = "Connection to " + host_ + ":" + std::to_string(port_) + " failed.";
        return connectStatus;
    }
    close(sock);
    return connectStatus;
}
