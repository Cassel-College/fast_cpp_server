#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <mosquitto.h>

#include "IMqttPublisher.hpp"

namespace my_mqtt {
using Handler = std::function<void(const std::string& topic, const std::string& payload)>;

class MqttService;

struct Route;
class PublisherAdapter;

/**
 * @brief MQTT 服务单例类
 */
class MqttService {
public:

    static MqttService& GetInstance();

    // cfg 示例：
    // {
    //   "host":"127.0.0.1",
    //   "port":1883,
    //   "keepalive":60,
    //   "client_id":"app_01",
    //   "clean_session":true,
    //   "username":"", "password":"",
    //   "reconnect": { "min_sec":2, "max_sec":32 }
    // }
    bool Init(const nlohmann::json& cfg);
    bool Start();   // connect + loop_start
    void Stop();    // disconnect + loop_stop

    bool IsRunning() const { return running_.load(); }

    /**
     * @brief 发布消息
     * 
     * @param topic 主题
     * @param payload 消息内容
     * @param qos 服务质量等级
     * @param retain 是否保留消息
     * @return true 
     * @return false 
     */
    bool Publish(const std::string& topic,
                 const std::string& payload,
                 int qos = 0,
                 bool retain = false);

    /**
     * @brief 订阅主题
     * 
     * @param topicFilter 主题过滤器
     * @param qos 服务质量等级
     * @return true 
     * @return false 
     */
    bool Subscribe(const std::string& topicFilter, int qos = 0);

    /**
     * @brief 添加路由，注册后自动订阅对应主题过滤器
     * 
     * @param topicFilter 主题过滤器
     * @param handler 消息处理回调
     * @param qos 服务质量等级
     */
    void AddRoute(std::string topicFilter, Handler handler, int qos = 0);

    /**
     * @brief 获取发布者接口，用于业务层注入
     * 
     * @return std::shared_ptr<IMqttPublisher> 
     */
    std::shared_ptr<IMqttPublisher> GetPublisher();

    const nlohmann::json GetConfig();
    const nlohmann::json GetRoutes();

private:
    MqttService() = default;                                // 私有构造函数
    ~MqttService();                                         // 私有析构函数
    MqttService(const MqttService&) = delete;               // 禁用拷贝构造
    MqttService& operator=(const MqttService&) = delete;    // 禁用赋值操作符

private:

private:
    // mosquitto callbacks
    static void on_connect_static(struct mosquitto* m, void* obj, int rc);                          // NOLINT
    static void on_disconnect_static(struct mosquitto* m, void* obj, int rc);                       // NOLINT
    static void on_message_static(struct mosquitto* m, void* obj, const mosquitto_message* msg);    // NOLINT
    static void on_log_static(struct mosquitto* m, void* obj, int level, const char* str);          // NOLINT
    void OnConnect(int rc);                                                                         // NOLINT
    void OnDisconnect(int rc);                                                                      // NOLINT
    void OnMessage(const mosquitto_message* msg);                                                   // NOLINT
    void DispatchRoutes(const std::string& topic, const std::string& payload);                      // NOLINT
    static bool MatchTopicFilter(const std::string& filter, const std::string& topic);              // NOLINT

private:
    std::mutex                              mtx_;                       // 保护 mosq_ / routes_ / config
    std::mutex                              pub_mtx_;                   // 保护 mosquitto_publish（线程安全）
    nlohmann::json                          cfg_;                       // 配置                   
    std::string                             host_;                      // MQTT 服务器地址
    int                                     port_{1883};                // MQTT 服务器端口
    int                                     keepalive_{60};             // 保持连接时间间隔               
    std::string                             client_id_;                 // 客户端 ID                 
    bool                                    clean_session_{true};       // 清理会话标志
    std::string                             username_;                  // 用户名
    std::string                             password_;                  // 密码
    int                                     reconnect_min_sec_{2};      // 重连最小间隔秒数
    int                                     reconnect_max_sec_{32};     // 重连最大间隔秒数
    std::atomic<bool>                       inited_{false};           // 是否已初始化
    std::atomic<bool>                       running_{false};          // 是否正在运行
    std::atomic<bool>                       connected_{false};        // 是否已连接
    struct mosquitto*                       mosq_{nullptr};             // mosquitto 客户端句柄
    std::vector<Route>                      routes_;                    // 路由列表
    std::shared_ptr<PublisherAdapter>       publisher_adapter_;         // 发布者适配器
};

} // namespace my_mqtt