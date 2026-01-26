#ifndef INIT_TOOLS_H
#define INIT_TOOLS_H
#pragma once
#include<iostream>
#include<string>
#include<vector>
#include<map>


namespace tools {
namespace init_tools {

    bool checkConfigLoadStatus(bool load_ini_config_status, bool load_json_config_status, bool load_yaml_config_status);

    void loadConfigFromArguments(
        const std::vector<std::map<std::string, std::string>>& args, 
        std::vector<std::string>& logInfos, 
        std::string& configFilePath);

    bool initLoadConfig(const std::string& type_,
        const std::string& config_file_path,
        std::vector<std::string>& logInfos);

    /**
     * @brief Get the Json Config Path From I N I Config object
     * 
     * @param {type} ini_config_file_path 
     * @param {type} json_config_path 
     * @param {type} default_json_config_path 
     * @param {type} logInfos 
     * @return true 
     * @return false 
     */
    bool getJsonConfigPathFromINIConfig(
        const std::string& ini_config_file_path,
        std::string& json_config_path,
        const std::string& default_json_config_path,
        std::vector<std::string>& logInfos);
    
    /**
     * @brief Get the Yaml Config Path From I N I Config object
     * 
     * @param {type} ini_config_file_path 
     * @param {type} yaml_config_path 
     * @param {type} default_yaml_config_path 
     * @param {type} logInfos 
     * @return true 
     * @return false 
     */
    bool getYamlConfigPathFromINIConfig(
        const std::string& ini_config_file_path,
        std::string& yaml_config_path,
        const std::string& default_yaml_config_path,
        std::vector<std::string>& logInfos);
};
};

#endif // INIT_TOOLS_H