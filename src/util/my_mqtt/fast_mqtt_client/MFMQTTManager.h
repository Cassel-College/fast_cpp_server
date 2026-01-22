#ifndef FAST_MQTT_MFMQTTTMANAGER_H
#define FAST_MQTT_MFMQTTTMANAGER_H

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "MFMQTTClient.h"

namespace fast_mqtt {

class MFMQTTManager {
public:
    bool add_client(const std::string& name, const nlohmann::json& cfg);
    bool remove_client(const std::string& name);
    std::shared_ptr<MFMQTTClient> get(const std::string& name);

    bool publish(const std::string& name, const std::string& topic, const std::string& payload, int qos = -1, bool retain = false);
    bool subscribe(const std::string& name, const std::string& topic, int qos = -1);

private:
    std::unordered_map<std::string, std::shared_ptr<MFMQTTClient>> clients_;
    std::mutex mtx_;
};

} // namespace fast_mqtt

#endif // FAST_MQTT_MFMQTTTMANAGER_H