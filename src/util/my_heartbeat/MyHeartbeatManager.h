#pragma once
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>
#include <mutex>

class HeartbeatManager {
public:
    static HeartbeatManager& Instance();

    void Init(const nlohmann::json& config);
    void Start();
    void Stop();

    /**
     * @brief Get the Heartbeat Snapshot object  / 获取心跳数据快照
     * 
     * @return nlohmann::json 
     */
    nlohmann::json GetHeartbeatSnapshot();
    
private:
    HeartbeatManager() = default;

    ~HeartbeatManager() { Stop(); }

    /**
     * @brief 心跳工作线程主循环
     * 
     */
    void WorkerLoop();

    /**
     * @brief 构建心跳数据
     * 
     */
    void BuildHeartbeat();

    /**
     * @brief 发送心跳数据
     * 
     * @param data 心跳数据
     */
    void SendHeartbeat(const nlohmann::json& data);

private:
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mutex mutex_;                  // 保护 heartbeat_data_ 的互斥锁
    nlohmann::json config_;             // 心跳配置
    nlohmann::json heartbeat_data_;     // 心跳数据
    int interval_sec_{5};               // 心跳间隔，默认5秒
    bool simple_json4log{false};        // 默认启用简化日志格式
    time_t start_time_{0};              // 启动时间

};
