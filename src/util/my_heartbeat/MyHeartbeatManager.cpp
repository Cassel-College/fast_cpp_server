#include "MyHeartbeatManager.h"
#include <unistd.h>
#include <ctime>
#include <mutex>
#include <chrono>
#include <random>

#include "MyLog.h"
#include "MyEdges.h"
#include "MqttService.hpp"


namespace my_heartbeat {

HeartbeatManager& HeartbeatManager::GetInstance() {
    static HeartbeatManager inst;
    return inst;
}

HeartbeatManager::HeartbeatManager() {
    MYLOG_INFO("HeartbeatManager 开始构造");
    SetPublisher();
    MYLOG_INFO("HeartbeatManager 构造完成");
}


void HeartbeatManager::SetPublisher() {
    MYLOG_INFO("MQTT publisher ------------------> HeartbeatManager");
    std::lock_guard<std::mutex> lock(mutex_);
    my_mqtt::MqttService& mqtt_service = my_mqtt::MqttService::GetInstance();
    publisher_ = std::move(mqtt_service.GetPublisher());
    if (!publisher_) {
        MYLOG_WARN("HeartbeatManager SetPublisher: publisher is null");
    } else {
        MYLOG_INFO("HeartbeatManager SetPublisher: publisher injected successfully");
    }
}

void HeartbeatManager::SetMqttPublishConfig(std::string topic_fmt, int qos, bool retain) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!topic_fmt.empty()) {
        topic_fmt_ = std::move(topic_fmt);
    }
    qos_ = qos;
    retain_ = retain;
    MYLOG_INFO("HeartbeatManager SetMqttPublishConfig: topic_fmt={}, qos={}, retain={}", topic_fmt_, qos_, retain_);
}

std::string HeartbeatManager::formatTopic(const std::string& fmt, const std::string& source) {
    std::string t = fmt;
    const std::string key = "{source}";
    size_t pos = 0;
    while ((pos = t.find(key, pos)) != std::string::npos) {
        t.replace(pos, key.length(), source);
        pos += source.length();
    }
    return t;
}
void HeartbeatManager::Init(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    config_ = config;
    MYLOG_INFO("初始化 HeartbeatManager 配置（业务配置）: {}", config_.dump(4));

    // interval / log format
    interval_sec_ = config_.value("interval_sec", 5);
    simple_json4log = config_.value("simple_json4log", false);

    // source_id
    source_id_ = config_.value("source_id", std::string());
    if (source_id_.empty()) {
        char hn[128] = {0};
        if (gethostname(hn, sizeof(hn)) == 0) source_id_ = hn;
        else source_id_ = "unknown";
    }

    // publish settings (业务层只关心 topic/qos/retain，不关心 MQTT 连接参数)
    topic_fmt_  = config_.value("topic_fmt", topic_fmt_);
    qos_        = config_.value("qos", qos_);
    retain_     = config_.value("retain", retain_);

    start_time_ = time(nullptr);

    MYLOG_INFO("HeartbeatManager initialized: interval_sec={}, source_id={}, topic_fmt={}, qos={}, retain={}",
               interval_sec_, source_id_, topic_fmt_, qos_, retain_);
}
void HeartbeatManager::Start() {
    if (running_.exchange(true)) {
        MYLOG_WARN("HeartbeatManager 已经在运行，忽略重复启动请求");
        return;
    }

    MYLOG_INFO("Starting HeartbeatManager worker thread...");
    worker_ = std::thread(&HeartbeatManager::WorkerLoop, this);
}

void HeartbeatManager::Stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    MYLOG_INFO("HeartbeatManager stopped");
}

void HeartbeatManager::WorkerLoop() {
    // initial jitter
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> jitter_ms(0, 3000);
    std::this_thread::sleep_for(std::chrono::milliseconds(jitter_ms(gen)));

    while (running_.load()) {
        try {
            BuildHeartbeat();
            SendHeartbeat(); // log
            SendOnceByMQTT();      // mqtt publish (if publisher injected)
        } catch (const std::exception& e) {
            MYLOG_ERROR("Heartbeat error: {}", e.what());
        } catch (...) {
            MYLOG_ERROR("Heartbeat unknown error");
        }

        for (int i = 0; i < interval_sec_ && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}


void HeartbeatManager::BuildHeartbeat() {
    // 不强行持锁整个函数，减少阻塞；只在写入 heartbeat_data_ 时加锁
    nlohmann::json base = config_.value("base", nlohmann::json::object());

    base["pid"] = getpid();
    base["timestamp"] = time(nullptr);
    base["uptime_sec"] = time(nullptr) - start_time_;

    nlohmann::json hb = nlohmann::json::object();
    hb["base"] = base;
    hb["extra"] = config_.value("extra", nlohmann::json::object());

    if (true) {
        heartbeat_data_["edge_summary"] = my_edge::MyEdges::GetInstance().GetHeartbeatInfo();
    }
    // finally update heartbeat_data_ with lock
    {
        std::lock_guard<std::mutex> lock(mutex_);
        heartbeat_data_ = std::move(hb);
    }
}

nlohmann::json HeartbeatManager::GetHeartbeatSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    return heartbeat_data_;
}

void HeartbeatManager::SendOnceByMQTT() {
    std::shared_ptr<my_mqtt::IMqttPublisher> pub;
    std::string topic_fmt;
    int qos;
    bool retain;
    std::string source;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pub = publisher_;
        topic_fmt = topic_fmt_;
        qos = qos_;
        retain = retain_;
        source = source_id_;
    }

    if (!pub) {
        MYLOG_WARN("HeartbeatManager SendOnce: publisher not injected, skip mqtt publish");
        return;
    } else {
        MYLOG_DEBUG("HeartbeatManager SendOnce: publisher available, proceed mqtt publish");
    }

    // build minimal payload
    nlohmann::json payload;
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    payload["version"]      = "1.0";
    payload["source"]       = source;
    payload["type"]         = "heartbeat";
    payload["timestamp"]    = ts;
    payload["seq"]          = ++seq_;
    payload["status"]       = "online";

    const std::string s = payload.dump();
    const std::string topic = formatTopic(topic_fmt, source);

    bool ok = false;
    try {
        
        // MyProto::Person ph;
        // ph.set_name("alice");
        // ph.set_id(123);
        // std::string pb_data;
        // ph.SerializeToString(&pb_data);
        // ok = pub->Publish(topic, pb_data, qos, retain);

        ok = pub->Publish(topic, s, qos, retain);
    } catch (const std::exception& e) {
        MYLOG_ERROR("Heartbeat Publish exception: topic={}, err={}", topic, e.what());
        ok = false;
    } catch (...) {
        MYLOG_ERROR("Heartbeat Publish unknown exception: topic={}", topic);
        ok = false;
    }

    if (!ok) {
        MYLOG_WARN("Heartbeat publish failed: topic={}", topic);
    } else {
        MYLOG_INFO("Heartbeat published: topic={}", topic);
    }
}

nlohmann::json HeartbeatManager::GetInitConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void HeartbeatManager::SendHeartbeat() {
    // 仍保留你原来的 log 行为（注意：heartbeat_data_ 结构已变为 {base, extra, edge_*}）
    std::string sender = config_.value("sender", "log");
    if (sender == "log") {
        nlohmann::json snapshot = GetHeartbeatSnapshot();
        if (simple_json4log) {
            // 简化打印：只打 base（你可按需调整）
            if (snapshot.contains("base")) {
                MYLOG_INFO("Heartbeat(base): {}", snapshot["base"].dump());
            } else {
                MYLOG_INFO("Heartbeat: {}", snapshot.dump());
            }
        } else {
            MYLOG_INFO("Heartbeat Data: {}", snapshot.dump(4));
        }
    }
    // http / mqtt 等扩展不要放在这里（mqtt 已在 SendOnce）
}


}; // namespace my_heartbeat