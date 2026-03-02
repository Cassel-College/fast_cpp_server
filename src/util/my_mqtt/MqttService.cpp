#include "MqttService.hpp"

#include <algorithm>
#include <sstream>

#include "MyLog.h"

namespace my_mqtt {

// namespace-level types (forward-declared in header)
struct Route {
    std::string filter;
    Handler handler;
    int qos{0};
};

class PublisherAdapter final : public IMqttPublisher {
public:
    explicit PublisherAdapter(MqttService& svc) : svc_(svc) {}
    bool Publish(const std::string& topic,
                 const std::string& payload,
                 int qos,
                 bool retain) override;
private:
    MqttService& svc_;
};

static std::atomic<bool> g_mosq_inited{false};

static std::vector<std::string> splitTopic(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '/')) out.push_back(item);
    return out;
}

MqttService& MqttService::GetInstance() {
    static MqttService inst;
    return inst;
}

MqttService::~MqttService() {
    try { 
        Stop();
     } catch (...) {
        MYLOG_ERROR("Exception caught in ~MqttService");
     }
}

bool MqttService::Init(const nlohmann::json& cfg) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (inited_.load()) {
        MYLOG_WARN("my_mqtt::MqttService already initialized");
        return true;
    }

    cfg_            = cfg;
    host_           = cfg_.value("host", std::string());
    port_           = cfg_.value("port", 1883);
    keepalive_      = cfg_.value("keepalive", 60);
    client_id_      = cfg_.value("client_id", std::string());
    clean_session_  = cfg_.value("clean_session", true);
    username_       = cfg_.value("username", std::string());
    password_       = cfg_.value("password", std::string());

    if (cfg_.contains("reconnect") && cfg_["reconnect"].is_object()) {
        reconnect_min_sec_ = cfg_["reconnect"].value("min_sec", reconnect_min_sec_);
        reconnect_max_sec_ = cfg_["reconnect"].value("max_sec", reconnect_max_sec_);
    }

    if (host_.empty()) {
        MYLOG_ERROR("my_mqtt::MqttService Init failed: host is empty");
        return false;
    }

    if (!g_mosq_inited.exchange(true)) {
        mosquitto_lib_init();
    }

    // 创建 mosquitto client；user data 指向 this
    mosq_ = mosquitto_new(client_id_.empty() ? nullptr : client_id_.c_str(),
                          clean_session_,
                          this);
    if (!mosq_) {
        MYLOG_ERROR("mosquitto_new failed");
        return false;
    }

    if (!username_.empty()) {
        int rc = mosquitto_username_pw_set(mosq_,
                                           username_.c_str(),
                                           password_.empty() ? nullptr : password_.c_str());
        if (rc != MOSQ_ERR_SUCCESS) {
            MYLOG_ERROR("mosquitto_username_pw_set failed: %s", mosquitto_strerror(rc));
            return false;
        }
    }

    // 设置自动重连退避
    mosquitto_reconnect_delay_set(mosq_,
                                  reconnect_min_sec_,
                                  reconnect_max_sec_,
                                  true);

    mosquitto_connect_callback_set(mosq_, &MqttService::on_connect_static);
    mosquitto_disconnect_callback_set(mosq_, &MqttService::on_disconnect_static);
    mosquitto_message_callback_set(mosq_, &MqttService::on_message_static);
    mosquitto_log_callback_set(mosq_, &MqttService::on_log_static);

    publisher_adapter_ = std::make_shared<PublisherAdapter>(*this);

    inited_.store(true);
    MYLOG_INFO("my_mqtt::MqttService Init ok host={} port={} client_id={}",
               host_, port_, client_id_);
    return true;
}

bool MqttService::Start() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!inited_.load() || !mosq_) {
        MYLOG_ERROR("my_mqtt::MqttService Start failed: not inited");
        return false;
    }
    if (running_.exchange(true)) {
        MYLOG_WARN("my_mqtt::MqttService already running");
        return true;
    }

    int rc = mosquitto_connect_async(mosq_, host_.c_str(), port_, keepalive_);
    if (rc != MOSQ_ERR_SUCCESS) {
        running_.store(false);
        MYLOG_ERROR("mosquitto_connect_async failed: {}", mosquitto_strerror(rc));
        return false;
    }

    rc = mosquitto_loop_start(mosq_);
    if (rc != MOSQ_ERR_SUCCESS) {
        running_.store(false);
        MYLOG_ERROR("mosquitto_loop_start failed: {}", mosquitto_strerror(rc));
        return false;
    }
    running_.store(true);
    MYLOG_INFO("my_mqtt::MqttService started");
    return true;
}

void MqttService::Stop() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!inited_.load()) return;

    running_.store(false);
    connected_.store(false);

    if (mosq_) {
        mosquitto_disconnect(mosq_);
        mosquitto_loop_stop(mosq_, true);
        mosquitto_destroy(mosq_);
        mosq_ = nullptr;
    }

    routes_.clear();
    publisher_adapter_.reset();

    // 注意：mosquitto_lib_cleanup 是全局的，通常进程退出时再调用
    // 这里不调用，避免多个模块/实例冲突

    inited_.store(false);
    MYLOG_INFO("my_mqtt::MqttService stopped");
}

std::shared_ptr<IMqttPublisher> MqttService::GetPublisher() {
    std::lock_guard<std::mutex> lk(mtx_);
    return publisher_adapter_;
}

bool MqttService::Publish(const std::string& topic,
                          const std::string& payload,
                          int qos,
                          bool retain) {
    if (!running_.load() || !mosq_) {
        MYLOG_WARN("Publish skipped: service not running");
        return false;
    }

    std::lock_guard<std::mutex> lk(pub_mtx_);
    int mid = 0;
    int rc = mosquitto_publish(mosq_,
                              &mid,
                              topic.c_str(),
                              static_cast<int>(payload.size()),
                              payload.data(),
                              qos,
                              retain);
    if (rc != MOSQ_ERR_SUCCESS) {
        MYLOG_ERROR("mosquitto_publish failed: {} topic={}", mosquitto_strerror(rc), topic);
        return false;
    }
    return true;
}

bool MqttService::Subscribe(const std::string& topicFilter, int qos) {
    if (!running_.load() || !mosq_) {
        MYLOG_WARN("Subscribe skipped: service not running");
        return false;
    }
    int rc = mosquitto_subscribe(mosq_, nullptr, topicFilter.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        MYLOG_ERROR("mosquitto_subscribe failed: {} filter={}", mosquitto_strerror(rc), topicFilter);
        return false;
    }
    return true;
}

void MqttService::AddRoute(std::string topicFilter, Handler handler, int qos) {
    if (!handler) {
        MYLOG_WARN("AddRoute ignored: empty handler filter={}", topicFilter);
        return;
    }

    {
        std::lock_guard<std::mutex> lk(mtx_);
        routes_.push_back(Route{topicFilter, std::move(handler), qos});
    }

    if (!Subscribe(topicFilter, qos)) {
        MYLOG_WARN("AddRoute: Subscribe failed filter={}", topicFilter);
    } else {
        MYLOG_INFO("AddRoute ok filter={} qos={}", topicFilter, qos);
    }
}

// static callbacks
void MqttService::on_connect_static(struct mosquitto* m, void* obj, int rc) {
    (void)m;
    auto* self = static_cast<MqttService*>(obj);
    if (self) self->OnConnect(rc);
}

void MqttService::on_disconnect_static(struct mosquitto* m, void* obj, int rc) {
    (void)m;
    auto* self = static_cast<MqttService*>(obj);
    if (self) self->OnDisconnect(rc);
}

void MqttService::on_message_static(struct mosquitto* m, void* obj, const mosquitto_message* msg) {
    (void)m;
    auto* self = static_cast<MqttService*>(obj);
    if (self) self->OnMessage(msg);
}

void MqttService::on_log_static(struct mosquitto* m, void* obj, int level, const char* str) {
    (void)m; (void)obj; (void)level;
    if (str) MYLOG_INFO("[mosquitto] {}", str);
}

// instance handlers
void MqttService::OnConnect(int rc) {
    connected_.store(rc == 0);
    if (rc == 0) {
        MYLOG_INFO("MQTT connected OK");
        // 连接成功后（或重连后），为已注册的 routes 自动订阅主题
        std::vector<Route> routes_copy;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            routes_copy = routes_;
        }
        for (const auto &r : routes_copy) {
            if (!mosq_) break;
            int sub_rc = mosquitto_subscribe(mosq_, nullptr, r.filter.c_str(), r.qos);
            if (sub_rc != MOSQ_ERR_SUCCESS) {
                MYLOG_WARN("Auto-subscribe failed for filter=%s rc=%s", r.filter.c_str(), mosquitto_strerror(sub_rc));
            } else {
                MYLOG_INFO("Auto-subscribe ok filter=%s qos=%d", r.filter.c_str(), r.qos);
            }
        }
    } else {
        MYLOG_WARN("MQTT connect failed rc={}", rc);
    }
}

void MqttService::OnDisconnect(int rc) {
    connected_.store(false);
    if (!running_.load()) {
        MYLOG_INFO("MQTT disconnected (service stopping) rc={}", rc);
        return;
    }
    MYLOG_WARN("MQTT disconnected rc={} (auto reconnect enabled via mosquitto)", rc);
}

void MqttService::OnMessage(const mosquitto_message* msg) {
    if (!msg || !msg->topic) return;

    std::string topic = msg->topic;
    std::string payload;
    if (msg->payload && msg->payloadlen > 0) {
        payload.assign(static_cast<const char*>(msg->payload),
                       static_cast<size_t>(msg->payloadlen));
    }

    DispatchRoutes(topic, payload);
}

void MqttService::DispatchRoutes(const std::string& topic, const std::string& payload) {
    std::vector<Route> routes_copy;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!running_.load()) return;
        routes_copy = routes_;
    }

    for (const auto& r : routes_copy) {
        if (MatchTopicFilter(r.filter, topic)) {
            try {
                r.handler(topic, payload);
            } catch (const std::exception& e) {
                MYLOG_ERROR("route handler exception filter={} err={}", r.filter, e.what());
            } catch (...) {
                MYLOG_ERROR("route handler unknown exception filter={}", r.filter);
            }
        }
    }
}

bool MqttService::MatchTopicFilter(const std::string& filter, const std::string& topic) {
    const auto f = splitTopic(filter);
    const auto t = splitTopic(topic);

    size_t i = 0;
    for (; i < f.size(); ++i) {
        const auto& fp = f[i];

        if (fp == "#") {
            return true; // match rest
        }

        if (i >= t.size()) return false;

        if (fp == "+") continue;

        if (fp != t[i]) return false;
    }

    return i == t.size();
}

// PublisherAdapter 方法实现
bool PublisherAdapter::Publish(const std::string& topic,
                              const std::string& payload,
                              int qos,
                              bool retain) {
    return svc_.Publish(topic, payload, qos, retain);
}


const nlohmann::json MqttService::GetConfig() {
    MYLOG_INFO("MqttService GetConfig called");
    try{
        std::lock_guard<std::mutex> lock(mtx_);
        MYLOG_INFO("MqttService config: {}", cfg_.dump(4));
        return cfg_;
    } catch (...) {
        MYLOG_WARN("MqttService GetConfig: service not initialized");
        return nlohmann::json();
    }
}

const nlohmann::json MqttService::GetRoutes() {
    MYLOG_INFO("MqttService GetRoutes called");
    try {
        std::lock_guard<std::mutex> lock(mtx_);
        nlohmann::json routes_json = nlohmann::json::array();
        for (const auto& r : routes_) {
            nlohmann::json route;
            route["filter"] = r.filter;
            route["qos"] = r.qos;
            routes_json.push_back(route);
        }
        return routes_json;

    } catch (...) {
        MYLOG_WARN("MqttService GetRoutes: service not initialized");
        return nlohmann::json();
    }
    
}

} // namespace my_mqtt