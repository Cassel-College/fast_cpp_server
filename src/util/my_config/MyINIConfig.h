#pragma once
#include <string>
#include <map>
#include <mutex>

class MyINIConfig {
public:
    static void Init(const std::string& config_file_path);
    static MyINIConfig& GetInstance();

    bool HasKey(const std::string& key) const;
    bool GetString(const std::string& key, const std::string& def, std::string& out) const;
    bool GetInt(const std::string& key, int def, int& out) const;
    bool GetDouble(const std::string& key, double def, double& out) const;
    bool GetBool(const std::string& key, bool def, bool& out) const;

    std::string ShowConfig() const;

private:
    MyINIConfig() = default;
    bool Load(const std::string& path);

    static MyINIConfig* instance_;
    static std::once_flag init_flag_;

    std::map<std::string, std::string> kv_;
};
