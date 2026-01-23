#ifndef DEVICE_ONLINE_MONITOR_H
#define DEVICE_ONLINE_MONITOR_H

#include <string>
#include <thread>
#include <mutex>
#include <atomic>

class DeviceOnlineMonitor {
public:

    DeviceOnlineMonitor(); // 默认构造
    DeviceOnlineMonitor(const std::string& host, int port, int interval_seconds = 5, int fail_threshold = 3);
    ~DeviceOnlineMonitor();

    bool init(const std::string& host, int port, int interval_seconds = 5, int fail_threshold = 3);

    void start();
    void stop();
    bool isOnline();

private:
    void monitorLoop();
    bool tryConnect();

    std::string host_;
    int port_;
    int interval_seconds_;
    int fail_threshold_;

    std::atomic<bool> running_;
    std::thread monitor_thread_;

    std::mutex mutex_;
    bool online_;

    bool initialized_;
};

#endif // DEVICE_ONLINE_MONITOR_H
