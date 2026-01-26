#include <unistd.h> // for access function
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <exception>
#include "InitTools.h"
#include "MyINIConfig.h"
#include "MyJSONConfig.h"
#include "MyYAMLConfig.h"


namespace tools {
namespace init_tools {  


    void loadConfigFromArguments(
        const std::vector<std::map<std::string, std::string>>& args, 
        std::vector<std::string>& logInfos, 
        std::string& configFilePath) {
        
        for (const auto& item : args) {
            if (item.at("key") == "--config" || item.at("key") == "-c") {
                try {
                    std::string configFile = item.at("value");
                    std::cout << "Using config file: " << configFile << std::endl;
                    logInfos.emplace_back("Using config file: " + configFile);
                    if (access(configFile.c_str(), F_OK) != 0) {
                        std::cerr << "Config file does not exist: " << configFile << std::endl;
                        logInfos.emplace_back("Config file does not exist: " + configFile);
                    } else {
                        logInfos.emplace_back("Config file exists: " + configFile);
                        configFilePath = configFile;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error processing config argument: " << e.what() << std::endl;
                }
            }
        }
    }

    bool initLoadConfig(const std::string& type_,
                        const std::string& config_file_path,
                        std::vector<std::string>& logInfos)
    {
        bool load_ok = false;

        if (config_file_path.empty()) {
            std::cout << "[" << type_ << "] Config file path:" << config_file_path << " is empty, skipping load." << std::endl;
            logInfos.emplace_back("[Config] ❌ 配置文件路径为空，跳过 " + type_ + " 配置加载: " + config_file_path);
            return false;
        }
        if (access(config_file_path.c_str(), F_OK) != 0) {
            std::cout << "[" << type_ << "] Config file does not exist: " << config_file_path << std::endl;
            logInfos.emplace_back("[Config] ❌ 配置文件不存在: " + config_file_path + "，跳过 " + type_ + " 配置加载");
            return false;
        }
        auto tryLoad = [&](const std::string& name, auto&& initFunc, auto&& showFunc) -> bool {
            try {
                initFunc();
                logInfos.emplace_back("[Config] " + name + " 配置加载成功");
                std::cout << "[" << name << "] Config Loaded Successfully." << std::endl;
                std::cout << showFunc() << std::endl;
                return true;
            } catch (const std::exception& e) {
                logInfos.emplace_back("[Config] " + name + " 配置加载失败: " + e.what());
            } catch (...) {
                logInfos.emplace_back("[Config] " + name + " 配置加载失败: unknown exception");
            }
            return false;
        };
        std::cout << "config_file_path: " << config_file_path << std::endl;
        // ========== INI ==========
        if (type_ == "ini" || type_ == "all") {
            load_ok |= tryLoad(
                "INI",
                [&]() { MyINIConfig::Init(config_file_path); },
                [&]() { return MyINIConfig::GetInstance().ShowConfig(); }
            );
        }

        // ========== JSON ==========
        if (type_ == "json" || type_ == "all") {
            load_ok |= tryLoad(
                "JSON",
                [&]() { MyJSONConfig::Init(config_file_path); },
                [&]() { return MyJSONConfig::GetInstance().ShowConfig(); }
            );
        }

        // ========== YAML ==========
        if (type_ == "yaml" || type_ == "all") {
            load_ok |= tryLoad(
                "YAML",
                [&]() { MyYAMLConfig::Init(config_file_path); },
                [&]() { return MyYAMLConfig::GetInstance().ShowConfig(); }
            );
        }

        if (!load_ok) {
            logInfos.emplace_back("[Config] ❌ 所有配置加载失败");
        }

        return load_ok;
    }

    bool getJsonConfigPathFromINIConfig(
        const std::string& ini_config_file_path,
        std::string& json_config_path,
        const std::string& default_json_config_path,
        std::vector<std::string>& logInfos) 
    {
        logInfos.emplace_back("[Config] 尝试从 INI 配置获取 JSON 配置路径...");
        bool load_ok = false;
        try {
            if (MyINIConfig::GetInstance().HasKey("config_dir")) {
                std::string config_dir = "";
                MyINIConfig::GetInstance().GetString("config_dir", default_json_config_path, config_dir);
                logInfos.emplace_back("[Config] 从 INI 配置获取到 config_dir: " + config_dir);
                // 判断目录是否存在
                if (access(config_dir.c_str(), F_OK) != 0) {
                    logInfos.emplace_back("[Config] ❌ config_dir 指定的目录不存在: " + config_dir);
                } else {
                    logInfos.emplace_back("[Config] config_dir 指定的目录存在: " + config_dir);
                    if (!config_dir.empty()) {
                        json_config_path = config_dir + "/config.json";
                        logInfos.emplace_back("[Config] 从 INI 配置获取到 JSON 配置路径: " + json_config_path);
                        if (access(json_config_path.c_str(), F_OK) != 0) {
                            logInfos.emplace_back("[Config] ❌ JSON 配置文件不存在: " + json_config_path);
                        } else {
                            logInfos.emplace_back("[Config] JSON 配置文件存在: " + json_config_path);
                            load_ok = true;
                        }
                    } else {
                        logInfos.emplace_back("[Config] ❌ config_dir 为空，无法构造 JSON 配置路径");
                    }
                }
            } else {
                logInfos.emplace_back("[Config] ❌ INI 配置中未找到 config_dir 键");
            }
            if (load_ok) {
                logInfos.emplace_back("[Config] 成功从 INI 配置获取 JSON 配置路径: " + json_config_path);
                return true;
            } else {
                json_config_path = default_json_config_path;
                logInfos.emplace_back("[Config] 使用默认 JSON 配置路径: " + json_config_path);
                return false;
            }
        } catch (const std::exception& e) {
            logInfos.emplace_back("[Config] ❌ 从 INI 配置获取 JSON 配置路径失败: " + std::string(e.what()));
            json_config_path = default_json_config_path;
            logInfos.emplace_back("[Config] 使用默认 JSON 配置路径: " + json_config_path);
            return false;
        }
    }
    

    bool getYamlConfigPathFromINIConfig(
        const std::string& ini_config_file_path,
        std::string& yaml_config_path,
        const std::string& default_yaml_config_path,
        std::vector<std::string>& logInfos) 
    {
        logInfos.emplace_back("[Config] 尝试从 INI 配置获取 YAML 配置路径...");
        bool load_ok = false;
        try {
            if (MyINIConfig::GetInstance().HasKey("config_dir")) {
                std::string config_dir = "";
                MyINIConfig::GetInstance().GetString("config_dir", default_yaml_config_path, config_dir);
                logInfos.emplace_back("[Config] 从 INI 配置获取到 config_dir: " + config_dir);
                // 判断目录是否存在
                if (access(config_dir.c_str(), F_OK) != 0) {
                    logInfos.emplace_back("[Config] ❌ config_dir 指定的目录不存在: " + config_dir);
                } else {
                    logInfos.emplace_back("[Config] config_dir 指定的目录存在: " + config_dir);
                    if (!config_dir.empty()) {
                        yaml_config_path = config_dir + "/config.yaml";
                        logInfos.emplace_back("[Config] 从 INI 配置获取到 YAML 配置路径: " + yaml_config_path);
                        if (access(yaml_config_path.c_str(), F_OK) != 0) {
                            logInfos.emplace_back("[Config] ❌ YAML 配置文件不存在: " + yaml_config_path);
                        } else {    
                            logInfos.emplace_back("[Config] YAML 配置文件存在: " + yaml_config_path);
                            load_ok = true;
                        }
                    } else {
                        logInfos.emplace_back("[Config] ❌ config_dir 为空，无法构造 YAML 配置路径");
                    }
                }
            } else {
                logInfos.emplace_back("[Config] ❌ INI 配置中未找到 config_dir 键");
            }
            if (load_ok) {
                logInfos.emplace_back("[Config] 成功从 INI 配置获取 YAML 配置路径: " + yaml_config_path);
                return true;
            } else {
                yaml_config_path = default_yaml_config_path;
                logInfos.emplace_back("[Config] 使用默认 YAML 配置路径: " + yaml_config_path);
                return false;
            }
        } catch (const std::exception& e) {
            logInfos.emplace_back("[Config] ❌ 从 INI 配置获取 YAML 配置路径失败: " + std::string(e.what()));
            yaml_config_path = default_yaml_config_path;
            logInfos.emplace_back("[Config] 使用默认 YAML 配置路径: " + yaml_config_path);
            return false;
        }
    }
};
};