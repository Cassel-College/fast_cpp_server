#pragma once
#include <vector>
#ifndef MY_API_H
#define MY_API_H

#include <thread>
#include <atomic>
#include <memory>
#include "MyLog.h"
#include <csignal>
#include <nlohmann/json.hpp>

#include "oatpp/core/base/Environment.hpp"
#include "oatpp/web/server/interceptor/RequestInterceptor.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp" 
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp-swagger/Controller.hpp"
#include "oatpp-swagger/Resources.hpp"

namespace my_api {

class MyAPI {
public:
    // 获取单例实例
    static MyAPI& GetInstance() {
        static MyAPI instance;
        return instance;
    }

    // 禁用拷贝构造和赋值
    MyAPI(const MyAPI&) = delete;
    MyAPI& operator=(const MyAPI&) = delete;

    // 生成启动参数配置文件
    void GenerateStartSettingByPipelineConfig(const nlohmann::json& pipeline_config);
    // 在独立线程中启动 API 服务
    void Start(int port = 8000);
    
    // 停止服务
    void Stop();
    bool IsRunning() const { return is_running_.load(); }


private:
    MyAPI() : is_running_(false) {
        api_enable_mapping_ = {}; // 初始化为空 JSON 对象
        loaded_models_      = {}; // 初始化为空模型列表
    }
    ~MyAPI() { Stop(); }

    void ServerThread_old(int port);
    void ServerThread(int port);

    std::atomic<bool> is_running_;
    std::thread server_thread_;

    bool getJsonBool(const nlohmann::json& j, const std::string& key, bool defaultValue = false);

    bool LoadAPIModel(
        std::shared_ptr<oatpp::web::server::HttpRouter> router,
        oatpp::web::server::api::Endpoints &docEndpoints, 
        std::shared_ptr<oatpp::data::mapping::ObjectMapper> objectMapper,
        std::string model_name = "default");

    nlohmann::json api_enable_mapping_;
    std::vector<std::string> loaded_models_;
};

void RunRestServer(int port);

} // namespace my_api

#endif