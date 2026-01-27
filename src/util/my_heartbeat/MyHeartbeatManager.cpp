#include "MyHeartbeatManager.h"
#include <unistd.h>
#include <iostream>
#include <ctime>
#include "MyEdges.h"
#include "MyLog.h"
#include "MyEdgeManager.h"
#include "IEdge.h"

using namespace edge_manager;

HeartbeatManager& HeartbeatManager::Instance() {
    static HeartbeatManager inst;
    return inst;
}

void HeartbeatManager::Init(const nlohmann::json& config) {
    config_ = config;
    MYLOG_INFO("初始化 HeartbeatManager 配置: {}", config_.dump(4));
    interval_sec_ = 5; // 如果不是对象，使用硬编码默认值
    try {
        interval_sec_ = config.value("interval_sec", 5);
        MYLOG_INFO("HeartbeatManager interval_sec set to {}", interval_sec_);
        simple_json4log = config.value("simple_json4log", false);
        MYLOG_INFO("HeartbeatManager config loaded: interval_sec = {}, simple_json4log = {}", 
                   interval_sec_, simple_json4log);
    } catch (const std::exception& e) {
        std::cout << "[HeartbeatManager] Failed to get interval_sec from global config: " << e.what() << std::endl;
    }
    start_time_ = time(nullptr);
    MYLOG_INFO("HeartbeatManager initialized with interval: {} seconds", interval_sec_);
}

void HeartbeatManager::Start() {
    if (running_) return;
    running_ = true;
    worker_ = std::thread(&HeartbeatManager::WorkerLoop, this);
}

void HeartbeatManager::Stop() {
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void HeartbeatManager::WorkerLoop() {
    while (running_) {
        try {
            auto hb = BuildHeartbeat();
            SendHeartbeat(hb);
        } catch (const std::exception& e) {
            MYLOG_ERROR("Heartbeat error: {}", e.what());
        }
        sleep(interval_sec_);
    }
}

nlohmann::json HeartbeatManager::BuildHeartbeat() const {
    nlohmann::json base = config_.value("base", nlohmann::json::object());

    base["pid"] = getpid();
    base["timestamp"] = time(nullptr);
    base["uptime_sec"] = time(nullptr) - start_time_;

    nlohmann::json heartbeat;
    heartbeat["base"] = base;
    heartbeat["extra"] = config_.value("extra", nlohmann::json::object());

    nlohmann::json edgeData = edge_manager::MyEdgeManager::GetInstance().ShowEdgesStatus();
    heartbeat["edge_devices"] = edgeData;

    // 添加边缘设备信息
    if (true) {
        heartbeat["edge_summary"] = my_edge::MyEdges::GetInstance().GetHeartbeatInfo();
    }

    return {{"heartbeat", heartbeat}};
}

void HeartbeatManager::SendHeartbeat(const nlohmann::json& data) {
    std::string sender = config_.value("sender", "log");
    if (sender == "log") {
        if (simple_json4log) {
            MYLOG_INFO("Heartbeat: {}", data["heartbeat"].dump());
        } else {
            MYLOG_INFO("Heartbeat Data: {}", data.dump(4));
        }
    }
    // http / mqtt 可在此扩展
}
