#pragma once
#ifndef MY_EDGE_MANAGER_H
#define MY_EDGE_MANAGER_H

#include "EdgeDevice.h"
#include <memory>
#include <shared_mutex>


namespace edge_manager {

class MyEdgeManager {
public:
    static MyEdgeManager& GetInstance() {
        static MyEdgeManager instance;
        return instance;
    }

    void Init(const nlohmann::json& args);
    
    // 单设备精细化管理
    void StartDevice(const std::string& name);
    void StopDevice(const std::string& name);
    void StartAll();
    void StopAll();

    // 状态查询
    nlohmann::json ShowEdgesStatus();
    std::string GetDeviceThreadInfo(const std::string& name);

    // 指令穿透
    bool ExecuteAction(const std::string& dev_name, const std::string& act_name, const nlohmann::json& p);

private:
    MyEdgeManager() = default;
    
    std::map<std::string, std::shared_ptr<EdgeDevice>> device_map_;
    std::shared_mutex rw_mutex_; // 读写锁：支持并发查询
};

} // namespace edge
#endif // MY_EDGE_MANAGER_H