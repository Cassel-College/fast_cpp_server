#include "EdgeDevice.h"
#include <chrono>
#include <sstream>

namespace edge_manager {

EdgeDevice::EdgeDevice(const nlohmann::json& config, int global_interval) {
    name_ = config.value("name", "unnamed");
    ip_ = config.value("ip", "127.0.0.1");
    port_ = config.value("port", 80);
    interval_sec_ = global_interval;

    if (config.contains("actions")) {
        for (auto it = config["actions"].begin(); it != config["actions"].end(); ++it) {
            action_map_[it.key()] = {it.key(), it.value().value("interface", "")};
        }
    }

    // 初始化在线监测器 (假设使用 53 端口或业务端口进行心跳检测)
    if (!online_monitor_.init(ip_, port_, 3, 3)) {
        MYLOG_ERROR("[Device:{}] OnlineMonitor 初始化失败", name_);
    } else {
        online_monitor_.start();
        MYLOG_INFO("[Device:{}] OnlineMonitor 已启动", name_);
    }
}

EdgeDevice::~EdgeDevice() {
    Stop();
}

void EdgeDevice::Start() {
    if (is_running_.exchange(true)) {
        MYLOG_WARN("[Device:{}] 线程已在运行中，无需重复启动", name_);
        return;
    }
    
    worker_thread_ = std::thread(&EdgeDevice::WorkerLoop, this);
    MYLOG_INFO("[Device:{}] 业务监控线程启动成功", name_);
}

void EdgeDevice::Stop() {
    is_running_ = false;
    if (worker_thread_.joinable()) {
        MYLOG_INFO("[Device:{}] 正在回收业务线程...", name_);
        worker_thread_.join();
        MYLOG_INFO("[Device:{}] 业务线程已安全停止", name_);
    }
}

void EdgeDevice::WorkerLoop() {
    while (is_running_) {
        // 1. 同步在线状态
        is_online_ = online_monitor_.isOnline(); // 假设 monitor 有此接口

        if (is_online_) {
            std::lock_guard<std::mutex> lock(biz_mutex_);
            biz_status_ = DeviceBusinessStatus::COLLECTING;
            
            MYLOG_DEBUG("[Device:{}] 正在执行周期性数据采集...", name_);
            // 模拟采集逻辑...
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
            
            biz_status_ = DeviceBusinessStatus::IDLE;
        } else {
            MYLOG_WARN("[Device:{}] 检测到离线，跳过本轮采集", name_);
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval_sec_));
    }
}

bool EdgeDevice::Execute(const std::string& action_name, const nlohmann::json& params) {
    if (!is_online_) {
        MYLOG_ERROR("[Device:{}] 执行失败: 设备离线", name_);
        return false;
    }

    std::lock_guard<std::mutex> lock(biz_mutex_);
    if (action_map_.find(action_name) == action_map_.end()) {
        MYLOG_ERROR("[Device:{}] 不支持动作: {}", name_, action_name);
        return false;
    }

    biz_status_ = DeviceBusinessStatus::EXECUTING;
    MYLOG_INFO("[Device:{}] >>> 开始执行动作: {}, 参数: {}", name_, action_name, params.dump());

    // 模拟执行过程
    std::this_thread::sleep_for(std::chrono::seconds(1)); 

    biz_status_ = DeviceBusinessStatus::IDLE;
    MYLOG_INFO("[Device:{}] <<< 动作执行完成: {}", name_, action_name);
    return true;
}

nlohmann::json EdgeDevice::GetFullStatus() {
    nlohmann::json status;
    status["name"] = name_;
    status["ip"] = ip_;
    status["online"] = is_online_.load();
    status["biz_status"] = StatusToString(biz_status_);
    status["thread_running"] = is_running_.load();
    status["thread_id"] = GetThreadId();
    return status;
}

std::string EdgeDevice::GetThreadId() {
    if (!worker_thread_.joinable()) return "none";
    std::stringstream ss;
    ss << worker_thread_.get_id();
    return ss.str();
}

std::string EdgeDevice::StatusToString(DeviceBusinessStatus s) {
    switch (s) {
        case DeviceBusinessStatus::IDLE: return "IDLE";
        case DeviceBusinessStatus::COLLECTING: return "COLLECTING";
        case DeviceBusinessStatus::EXECUTING: return "EXECUTING";
        case DeviceBusinessStatus::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

} // namespace edge