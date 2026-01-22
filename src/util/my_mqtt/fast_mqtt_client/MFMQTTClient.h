#pragma once
#ifndef FAST_MQTT_MFMQTTTCLIENT_H
#define FAST_MQTT_MFMQTTTCLIENT_H


#include "MyLog.h"
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <stdexcept>


namespace fast_mqtt {

class MFMQTTClient {
public:
    MFMQTTClient();
    ~MFMQTTClient();
    void init(const nlohmann::json& config);
    void set_message_handler(std::function<void(const std::string&, const std::string&)> cb);
    bool connect();
    void disconnect();
    bool publish(const std::string& topic, const std::string& payload, int qos = -1, bool retain = false);
    bool subscribe(const std::string& topic, int qos = -1);
    static void on_connect_static(struct mosquitto* m, void* obj, int rc);
    static void on_disconnect_static(struct mosquitto* m, void* obj, int rc);
    static void on_message_static(struct mosquitto* m, void* obj, const mosquitto_message* msg);
    static void on_log_static(struct mosquitto* m, void* obj, int level, const char* str);
    void on_connect(int rc);
    void on_disconnect(int rc);
    void on_message(const mosquitto_message* msg);
    void start_loop();
    void schedule_reconnect();
private:
    struct mosquitto* mosq_;
    std::string host_;
    int port_;
    int keepalive_;
    std::string client_id_;
    int qos_;
    bool clean_session_;
    std::string username_;
    std::string password_;

    std::atomic<bool> connected_;
    std::atomic<bool> stop_;
    std::thread loop_thread_;
    std::mutex loop_mtx_;
    std::mutex pub_mtx_;
    std::function<void(const std::string&, const std::string&)> msg_cb_;

    unsigned backoff_base_sec_;
    unsigned max_backoff_sec_;
    unsigned current_backoff_{2};

    static inline std::atomic<bool> mosquitto_initialized_{false};
};

} // namespace fast_mqtt

#endif // FAST_MQTT_MFMQTTTCLIENT_H