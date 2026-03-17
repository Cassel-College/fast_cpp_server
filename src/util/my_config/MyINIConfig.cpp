#include "MyINIConfig.h"
#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>

namespace {

std::string TrimCopy(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(begin, end - begin);
}

} // namespace

std::atomic<MyINIConfig*> MyINIConfig::instance_{nullptr};
std::once_flag MyINIConfig::init_flag_;

void MyINIConfig::Init(const std::string& path) {
    std::call_once(init_flag_, [&]() {
        std::unique_ptr<MyINIConfig> instance(new MyINIConfig());
        instance->Load(path);
        instance_.store(instance.release(), std::memory_order_release);
    });
}

MyINIConfig& MyINIConfig::GetInstance() {
    static MyINIConfig empty_instance;

    MyINIConfig* instance = instance_.load(std::memory_order_acquire);
    return instance != nullptr ? *instance : empty_instance;
}

bool MyINIConfig::IsInitialized() {
    return instance_.load(std::memory_order_acquire) != nullptr;
}

bool MyINIConfig::Load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    std::string line;
    while (std::getline(ifs, line)) {
        const std::string trimmed_line = TrimCopy(line);
        if (trimmed_line.empty() || trimmed_line[0] == '#' || trimmed_line[0] == ';') continue;

        auto pos = trimmed_line.find('=');
        if (pos == std::string::npos) continue;

        kv_[TrimCopy(trimmed_line.substr(0, pos))] = TrimCopy(trimmed_line.substr(pos + 1));
    }
    return true;
}

bool MyINIConfig::GetString(const std::string& key, const std::string& def, std::string& out) const {
    auto it = kv_.find(key);
    out = (it != kv_.end()) ? it->second : def;
    return it != kv_.end();
}

bool MyINIConfig::GetInt(const std::string& key, int def, int& out) const {
    auto it = kv_.find(key);
    out = it != kv_.end() ? std::stoi(it->second) : def;
    return it != kv_.end();
}

bool MyINIConfig::GetDouble(const std::string& key, double def, double& out) const {
    auto it = kv_.find(key);
    out = it != kv_.end() ? std::stod(it->second) : def;
    return it != kv_.end();
}

bool MyINIConfig::GetBool(const std::string& key, bool def, bool& out) const {
    auto it = kv_.find(key);
    out = it != kv_.end() ? (it->second == "true" || it->second == "1") : def;
    return it != kv_.end();
}

std::string MyINIConfig::ShowConfig() const {
    std::ostringstream oss;
    for (auto& kv : kv_) oss << kv.first << "=" << kv.second << "\n";
    return oss.str();
}

bool MyINIConfig::HasKey(const std::string& key) const {
    return kv_.find(key) != kv_.end();
}