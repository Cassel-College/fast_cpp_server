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


};
};