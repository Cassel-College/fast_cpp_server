#include "MFSMQTTClient.h"
#include <functional>
#include <memory>

namespace fast_mqtt {

MFSMQTTClient& MFSMQTTClient::instance() {
    static MFSMQTTClient inst;
    return inst;
}

void MFSMQTTClient::init(const nlohmann::json& cfg) {
    std::lock_guard<std::mutex> lk(mtx_);
    client_ = std::make_shared<MFMQTTClient>();
    client_->init(cfg);
}

bool MFSMQTTClient::publish(const std::string& topic, const std::string& payload, int qos, bool retain) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!client_) return false;
    return client_->publish(topic, payload, qos, retain);
}

bool MFSMQTTClient::subscribe(const std::string& topic, int qos) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!client_) return false;
    return client_->subscribe(topic, qos);
}

void MFSMQTTClient::set_message_handler(std::function<void(const std::string&, const std::string&)> cb) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (client_) client_->set_message_handler(std::move(cb));
}

} // namespace fast_mqtt
