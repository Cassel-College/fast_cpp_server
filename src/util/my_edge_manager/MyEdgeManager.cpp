#include "MyEdgeManager.h"

namespace edge_manager {

void MyEdgeManager::Init(const nlohmann::json& args) {
    MYLOG_INFO("===== MyEdgeManager 批量初始化开始 =====");
    try {
        int interval = args.value("interval_sec_", 5);
        auto edges = args.at("edges");

        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        for (auto it = edges.begin(); it != edges.end(); ++it) {
            auto dev = std::make_shared<EdgeDevice>(it.value(), interval);
            device_map_[it.value().value("name", "")] = dev;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("Manager 初始化崩溃: {}", e.what());
    }
}

nlohmann::json MyEdgeManager::ShowEdgesStatus() {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    nlohmann::json report = nlohmann::json::array();
    
    for (auto& pair : device_map_) {
        report.push_back(pair.second->GetFullStatus());
    }
    
    MYLOG_INFO("[StatusReport] 当前边缘设备快照: {}", report.dump());
    return report;
}

void MyEdgeManager::StartAll() {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    for (auto& pair : device_map_) {
        pair.second->Start();
    }
}

void MyEdgeManager::StopAll() {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    for (auto& pair : device_map_) {
        pair.second->Stop();
    }
}

void MyEdgeManager::StartDevice(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    if (device_map_.count(name)) {
        device_map_[name]->Start();
    } else {
        MYLOG_ERROR("无法启动: 未找到设备 {}", name);
    }
}

void MyEdgeManager::StopDevice(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    if (device_map_.count(name)) {
        device_map_[name]->Stop();
    }
}

bool MyEdgeManager::ExecuteAction(const std::string& dev_name, const std::string& act_name, const nlohmann::json& p) {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    if (device_map_.count(dev_name)) {
        return device_map_[dev_name]->Execute(act_name, p);
    }
    return false;
}

} // namespace edge