#include "MyINIConfig.h"
#include <fstream>
#include <sstream>

MyINIConfig* MyINIConfig::instance_ = nullptr;
std::once_flag MyINIConfig::init_flag_;

void MyINIConfig::Init(const std::string& path) {
    std::call_once(init_flag_, [&]() {
        instance_ = new MyINIConfig();
        instance_->Load(path);
    });
}

MyINIConfig& MyINIConfig::GetInstance() {
    return *instance_;
}

bool MyINIConfig::Load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        kv_[line.substr(0, pos)] = line.substr(pos + 1);
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