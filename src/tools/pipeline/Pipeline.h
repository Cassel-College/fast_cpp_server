#pragma once
#ifndef PIPELINE_H
#define PIPELINE_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include "MyLog.h"

namespace tools {
namespace pipeline {

class Pipeline {
public:
    static Pipeline& GetInstance() {
        static Pipeline instance;
        return instance;
    }

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    // 初始化：读取你提供的具体 JSON 结构并记录日志
    void Init(const nlohmann::json& config);

    // 启动：遍历 executes 列表，手动分发任务
    void Start();

    // 停止：优雅关闭所有线程
    void Stop();

private:
    Pipeline() : is_running_(false) {}
    ~Pipeline();

    // 递归解析 JSON 参数并按格式打印日志
    void LogRecursive(const std::string& prefix, const nlohmann::json& j);
    void LogArg(const std::string& name, const std::string& value);

    // --- 各个模块独立的启动函数 (你可以在这里编写具体的业务逻辑) ---
    void LaunchHeartbeat(const nlohmann::json& args);
    void LaunchComm(const nlohmann::json& args);
    void LaunchSystemHealthy(const nlohmann::json& args);
    void LaunchEdgeMonitor(const nlohmann::json& args);
    void LaunchRestAPI(const nlohmann::json& args);
    void LaunchEdge(const nlohmann::json& args);
    
    nlohmann::json config_data_;
    std::atomic<bool> is_running_;
    std::vector<std::thread> workers_;
};

} // namespace pipeline
} // namespace tools

#endif // PIPELINE_H