#pragma once
#ifndef EDGE_DEVICE_H
#define EDGE_DEVICE_H

#include <string>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>
#include "MyLog.h"
#include "device_online_monitor/DeviceOnlineMonitor.h" // 引入您实现的在线检查器

namespace edge_manager {

struct EdgeAction {
    std::string name;
    std::string interface;
};

// 设备业务姿态枚举
enum class DeviceBusinessStatus {
    IDLE,           // 空闲
    COLLECTING,     // 正在采集数据
    EXECUTING,      // 正在执行特定动作
    FAULT           // 业务故障
};

class EdgeDevice {
public:
    EdgeDevice(const nlohmann::json& config, int global_interval);
    ~EdgeDevice();

    // --- 控制接口 ---
    void Start();           // 启动业务监控线程
    void Stop();            // 停止业务监控线程
    bool Execute(const std::string& action_name, const nlohmann::json& params);

    // --- 状态接口 ---
    nlohmann::json GetFullStatus();
    bool IsOnline() const { return is_online_.load(); }
    std::string GetThreadId(); 

private:
    void WorkerLoop();      // 内部线程循环
    std::string StatusToString(DeviceBusinessStatus s);

    // 基础配置
    std::string name_;
    std::string ip_;
    int port_;
    int interval_sec_;
    std::map<std::string, EdgeAction> action_map_;

    // 状态变量
    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_online_{false};
    DeviceBusinessStatus biz_status_{DeviceBusinessStatus::IDLE};
    
    // 核心组件
    std::thread worker_thread_;
    DeviceOnlineMonitor online_monitor_; // 集成在线检查器
    std::mutex biz_mutex_;               // 保护业务姿态切换
};

} // namespace edge_manager
#endif