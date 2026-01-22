#pragma once
#ifndef FAST_MQTT_MFMQTTTSCLIENT_H
#define FAST_MQTT_MFMQTTTSCLIENT_H

#include <iostream>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <mutex>
#include <functional>
#include "MFMQTTClient.h"

namespace fast_mqtt {

class MFSMQTTClient {
public:
    static MFSMQTTClient& instance();

    void init(const nlohmann::json& cfg);
    bool publish(const std::string& topic, const std::string& payload, int qos = -1, bool retain = false);
    bool subscribe(const std::string& topic, int qos = -1);
    void set_message_handler(std::function<void(const std::string&, const std::string&)> cb);

private:
    MFSMQTTClient() = default;
    MFSMQTTClient(const MFSMQTTClient&) = delete;
    MFSMQTTClient& operator=(const MFSMQTTClient&) = delete;

    std::shared_ptr<MFMQTTClient> client_;
    std::mutex mtx_;
};

} // namespace fast_mqtt

#endif // FAST_MQTT_MFMQTTTSCLIENT_H