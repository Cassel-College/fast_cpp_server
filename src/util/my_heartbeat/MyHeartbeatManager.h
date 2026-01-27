#pragma once
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>

class HeartbeatManager {
public:
    static HeartbeatManager& Instance();

    void Init(const nlohmann::json& config);
    void Start();
    void Stop();

private:
    HeartbeatManager() = default;
    void WorkerLoop();
    nlohmann::json BuildHeartbeat() const;
    void SendHeartbeat(const nlohmann::json& data);

private:
    std::atomic<bool> running_{false};
    std::thread worker_;

    nlohmann::json config_;             // 心跳配置
    int interval_sec_{5};               // 心跳间隔，默认5秒
    bool simple_json4log{false};        // 默认启用简化日志格式
    time_t start_time_{0};              // 启动时间
};
