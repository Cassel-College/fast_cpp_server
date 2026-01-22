#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <sys/types.h> // for pid_t

namespace fast_mqtt {

class MFMQTTServer {
public:
    void init(const nlohmann::json& cfg);
    bool start();
    bool stop();
    std::optional<pid_t> pid() const { return pid_; }

private:
    std::string mosquitto_path_ = "mosquitto";
    std::string conf_path_;
    std::string log_path_;
    std::optional<pid_t> pid_;
    nlohmann::json cfg_;

    std::string expand_env(const std::string& s);
    bool write_conf();
};

} // namespace fast_mqtt
