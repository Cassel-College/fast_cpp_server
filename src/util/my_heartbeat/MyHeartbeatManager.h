#pragma once
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>
#include <mutex>
#include <vector>
#include <memory>
#include <string>

#include "IMqttPublisher.hpp"

namespace my_heartbeat {

class HeartbeatManager {
public:
    static HeartbeatManager& GetInstance();

    void Init(const nlohmann::json& config);

    /**
     * @brief Set the Publisher object
     * 
     * @param publisher 共享指针，指向已连接的 IMqttPublisher 实例
     */
    void SetPublisher();

    /**
     * @brief Set the Mqtt Publish Config object
     * 
     * @param topic_fmt 主题格式字符串，支持 {source} 占位符
     * @param qos MQTT QoS 等级，默认 1
     * @param retain 是否保留消息，默认 false
     */
    void SetMqttPublishConfig(std::string topic_fmt, int qos = 1, bool retain = false);

    void Start();
    void Stop();
    
    /**
     * @brief Get the Heartbeat Snapshot object  / 获取心跳数据快照
     * 
     * @return nlohmann::json 
     */
    nlohmann::json GetHeartbeatSnapshot();
    nlohmann::json GetInitConfig();
    
private:
    HeartbeatManager();
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
    void SendHeartbeat();

    /**
     * @brief 发送一次心跳（内部使用）
     * 
     */
    void SendOnceByMQTT();

    // helper: format topic string by replacing {source}
    static std::string formatTopic(const std::string& fmt, const std::string& source);

private:
    std::atomic<bool> running_{false};                            // 线程运行状态
    std::thread worker_;                                            // 心跳工作线程
    std::shared_ptr<my_mqtt::IMqttPublisher> publisher_{nullptr};            // MQTT 发布器
    std::string topic_fmt_{"system/heartbeats"};                    // 主题格式
    int qos_{1};                                                    // MQTT QoS 等级
    bool retain_{false};                                            // 是否保留消息
    uint64_t seq_{0};                                               // 心跳序列号
    std::mutex mutex_;                                              // 保护 heartbeat_data_ / config_ / publisher_ 等
    nlohmann::json config_;                                         // 心跳配置（业务配置）
    nlohmann::json heartbeat_data_;                                 // 心跳数据
    int interval_sec_{5};                                           // 心跳间隔秒数
    bool simple_json4log{false};                                    // 日志简化输出
    time_t start_time_{0};                                          // 启动时间
    std::string source_id_;                                         // 心跳源 ID

}; // class HeartbeatManager
}; // namespace my_heartbeat