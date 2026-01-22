#include <nlohmann/json.hpp>
#include <memory>
#include <utility>

#include "MyLog.h"
// #include "MFMQTTClient.h"   // 新增：需要完整类型以调用成员函数
#include "fast_mqtt_client/MFMQTTClient.h"
#include "fast_mqtt_client/MFMQTTManager.h"


namespace fast_mqtt {

bool MFMQTTManager::add_client(const std::string& name, const nlohmann::json& cfg) {
    try {
        std::lock_guard<std::mutex> lk(mtx_);
        if (clients_.count(name)) {
            MYLOG_WARN("client already exists: %s", name.c_str());
            return false;
        }
        std::shared_ptr<MFMQTTClient> cli = std::make_shared<MFMQTTClient>();

        cli->init(cfg);
        clients_[name] = cli;
        MYLOG_INFO("add_client ok: %s", name.c_str());
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("add_client exception: %s", e.what());
        return false;
    }
}

bool MFMQTTManager::remove_client(const std::string& name) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = clients_.find(name);
    if (it == clients_.end()) return false;
    clients_.erase(it);
    MYLOG_INFO("remove_client ok: %s", name.c_str());
    return true;
}

std::shared_ptr<MFMQTTClient> MFMQTTManager::get(const std::string& name) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = clients_.find(name);
    if (it == clients_.end()) return {};
    return it->second;
}

bool MFMQTTManager::publish(const std::string& name, const std::string& topic, const std::string& payload, int qos, bool retain) {
    auto cli = get(name);
    if (!cli) {
        return false;
    }
    return cli->publish(topic, payload, qos, retain);
}

bool MFMQTTManager::subscribe(const std::string& name, const std::string& topic, int qos) {
    auto cli = get(name);
    if (!cli) return false;
    return cli->subscribe(topic, qos);
}

} // namespace fast_mqtt
