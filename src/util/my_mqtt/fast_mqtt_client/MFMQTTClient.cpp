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

#include "fast_mqtt_client/MFMQTTClient.h"

namespace fast_mqtt {

    
    MFMQTTClient::MFMQTTClient() : mosq_(nullptr),
          connected_(false),
          stop_(false),
          clean_session_(true),
          keepalive_(60),
          qos_(0),
          backoff_base_sec_(2),
          max_backoff_sec_(32) {}


    MFMQTTClient::~MFMQTTClient() {
        try {
            disconnect();
            stop_ = true;
            {
                std::unique_lock<std::mutex> lk(loop_mtx_);
            }
            if (loop_thread_.joinable()) loop_thread_.join();
            if (mosq_) {
                mosquitto_destroy(mosq_);
                mosq_ = nullptr;
            }
            mosquitto_lib_cleanup();
        } catch (...) {
            // 防止析构抛异常
        }
    }

    void MFMQTTClient::init(const nlohmann::json& config) {
        try {
            host_ = config.value("host", "");
            port_ = config.value("port", 1883);
            keepalive_ = config.value("keepalive", 60);
            client_id_ = config.value("client_id", "");
            qos_ = config.value("qos", 0);
            clean_session_ = config.value("clean_session", true);
            username_ = config.value("username", "");
            password_ = config.value("password", "");

            if (host_.empty()) throw std::invalid_argument("host is required");
            if (!mosquitto_initialized_) {
                mosquitto_lib_init();
                mosquitto_initialized_ = true;
            }

            mosq_ = mosquitto_new(client_id_.empty() ? nullptr : client_id_.c_str(), clean_session_, this);
            if (!mosq_) throw std::runtime_error("mosquitto_new failed");

            if (!username_.empty())
                mosquitto_username_pw_set(mosq_, username_.c_str(), password_.empty() ? nullptr : password_.c_str());

            mosquitto_connect_callback_set(mosq_, &MFMQTTClient::on_connect_static);
            mosquitto_disconnect_callback_set(mosq_, &MFMQTTClient::on_disconnect_static);
            mosquitto_message_callback_set(mosq_, &MFMQTTClient::on_message_static);
            mosquitto_log_callback_set(mosq_, &MFMQTTClient::on_log_static);

            start_loop();
            MYLOG_INFO("MQTT client initialized host=%s port=%d client_id=%s", host_.c_str(), port_, client_id_.c_str());
        } catch (const std::exception& e) {
            MYLOG_ERROR("init error: %s", e.what());
            throw;
        }
    }

    void MFMQTTClient::set_message_handler(std::function<void(const std::string&, const std::string&)> cb) {
        msg_cb_ = std::move(cb);
    }

    bool MFMQTTClient::connect() {
        try {
            int rc = mosquitto_connect_async(mosq_, host_.c_str(), port_, keepalive_);
            if (rc != MOSQ_ERR_SUCCESS) {
                MYLOG_ERROR("connect_async failed rc=%d", rc);
                return false;
            }
            // 触发后台 loop 处理
            return true;
        } catch (const std::exception& e) {
            MYLOG_ERROR("connect exception: %s", e.what());
            return false;
        }
    }

    void MFMQTTClient::disconnect() {
        try {
            stop_ = true;
            if (mosq_) mosquitto_disconnect(mosq_);
        } catch (...) {
            MYLOG_WARN("disconnect exception ignored");
        }
    }

    bool MFMQTTClient::publish(const std::string& topic, const std::string& payload, int qos, bool retain) {
        std::lock_guard<std::mutex> lk(pub_mtx_);
        int use_qos = (qos >= 0 ? qos : qos_);
        if (!mosq_) return false;
        int mid = 0;
        int rc = mosquitto_publish(mosq_, &mid, topic.c_str(), (int)payload.size(), payload.data(), use_qos, retain);
        if (rc != MOSQ_ERR_SUCCESS) {
            MYLOG_ERROR("publish failed rc=%d topic=%s", rc, topic.c_str());
            return false;
        }
        MYLOG_INFO("publish ok mid=%d topic=%s len=%zu", mid, topic.c_str(), payload.size());
        return true;
    }

    bool MFMQTTClient::subscribe(const std::string& topic, int qos) {
        int use_qos = (qos >= 0 ? qos : qos_);
        int rc = mosquitto_subscribe(mosq_, nullptr, topic.c_str(), use_qos);
        if (rc != MOSQ_ERR_SUCCESS) {
            MYLOG_ERROR("subscribe failed rc=%d topic=%s", rc, topic.c_str());
            return false;
        }
        MYLOG_INFO("subscribe ok topic=%s qos=%d", topic.c_str(), use_qos);
        return true;
    }

    void MFMQTTClient::on_connect_static(struct mosquitto* m, void* obj, int rc) {
        auto* self = static_cast<MFMQTTClient*>(obj);
        self->on_connect(rc);
    }
    
    void MFMQTTClient::on_disconnect_static(struct mosquitto* m, void* obj, int rc) {
        auto* self = static_cast<MFMQTTClient*>(obj);
        self->on_disconnect(rc);
    }
    
    void MFMQTTClient::on_message_static(struct mosquitto* m, void* obj, const mosquitto_message* msg) {
        auto* self = static_cast<MFMQTTClient*>(obj);
        self->on_message(msg);
    }
    
    void MFMQTTClient::on_log_static(struct mosquitto* m, void* obj, int level, const char* str) {
        (void)m; (void)obj; (void)level;
        MYLOG_INFO("mosquitto: %s", str ? str : "");
    }

    void MFMQTTClient::on_connect(int rc) {
        connected_ = (rc == 0);
        MYLOG_INFO("on_connect rc=%d host=%s port=%d", rc, host_.c_str(), port_);
        if (connected_) {
            // 重置退避
            current_backoff_ = backoff_base_sec_;
        }
    }

    void MFMQTTClient::on_disconnect(int rc) {
        connected_ = false;
        if (stop_) {
            MYLOG_INFO("on_disconnect: stopped rc=%d", rc);
            return;
        }
        MYLOG_WARN("on_disconnect rc=%d, scheduling reconnect", rc);
        schedule_reconnect();
    }

    void MFMQTTClient::on_message(const mosquitto_message* msg) {
        try {
            if (!msg) return;
            std::string topic = msg->topic ? msg->topic : "";
            std::string payload;
            if (msg->payload && msg->payloadlen > 0)
                payload.assign(static_cast<const char*>(msg->payload), (size_t)msg->payloadlen);
            MYLOG_INFO("message arrived topic=%s len=%d", topic.c_str(), msg->payloadlen);
            if (msg_cb_) msg_cb_(topic, payload);
        } catch (const std::exception& e) {
            MYLOG_ERROR("on_message exception: %s", e.what());
        }
    }

    void MFMQTTClient::start_loop() {
        stop_ = false;
        current_backoff_ = backoff_base_sec_;
        loop_thread_ = std::thread([this]() {
            // 非阻塞主逻辑，内部处理自动重连与网络事件
            int rc = mosquitto_loop_start(mosq_);
            if (rc != MOSQ_ERR_SUCCESS) {
                MYLOG_ERROR("loop_start failed rc=%d", rc);
                return;
            }
            // 初始连接
            connect();
            while (!stop_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            mosquitto_loop_stop(mosq_, true);
        });
    }

    void MFMQTTClient::schedule_reconnect() {
        // 指数退避，最大上限
        unsigned wait_sec = current_backoff_;
        if (wait_sec > max_backoff_sec_) wait_sec = max_backoff_sec_;
        MYLOG_WARN("reconnect in %u seconds", wait_sec);
        std::thread([this, wait_sec]() {
            std::this_thread::sleep_for(std::chrono::seconds(wait_sec));
            if (stop_) return;
            int rc = mosquitto_reconnect_async(mosq_);
            if (rc != MOSQ_ERR_SUCCESS) {
                MYLOG_ERROR("reconnect_async failed rc=%d", rc);
            } else {
                MYLOG_INFO("reconnect_async triggered");
            }
        }).detach();
        // 增加退避
        current_backoff_ = std::min(current_backoff_ * 2, max_backoff_sec_);
    }

} // namespace fast_mqtt
