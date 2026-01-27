#include "MyAPI.h"

#include "controller/demo/edge_manager/EdgeController.hpp"
#include "controller/heartbeat_manager/HeartBeatController.h"
#include "controller/demo/edges/EdgesController.hpp"


// #include "oatpp/json/ObjectMapper.hpp" 
#include "oatpp/parser/json/mapping/ObjectMapper.hpp" 
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp-swagger/Controller.hpp"
#include "oatpp-swagger/Resources.hpp"
// #include "oatpp/Environment.hpp"
#include "oatpp/core/base/Environment.hpp"
#include "oatpp/web/server/interceptor/RequestInterceptor.hpp"
#include "MyINIConfig.h"
#include "MyLog.h"
#include <filesystem>

namespace my_api {

// --- 1. 定义 CORS 拦截器 ---
class CorsInterceptor : public oatpp::web::server::interceptor::RequestInterceptor {
public:
    std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest>& request) override {
        // 检查是否为 OPTIONS 预检请求
        auto method = request->getStartingLine().method;
        if (method == "OPTIONS") {
            auto response = OutgoingResponse::createShared(oatpp::web::protocol::http::Status::CODE_200, nullptr);
            response->putHeader("Access-Control-Allow-Origin", "*");
            response->putHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            response->putHeader("Access-Control-Allow-Headers", "DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization");
            return response;
        }
        return nullptr; // 其他请求继续后续路由
    }
};

// --- 2. 定义 Response 拦截器 (确保普通请求也带上跨域头) ---
class CorsResponseInterceptor : public oatpp::web::server::interceptor::ResponseInterceptor {
public:
    std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest>& request, 
                                               const std::shared_ptr<OutgoingResponse>& response) override {
        response->putHeader("Access-Control-Allow-Origin", "*");
        return response;
    }
};

void MyAPI::Start(int port) {
    if (is_running_.exchange(true)) return;
    server_thread_ = std::thread(&MyAPI::ServerThread, this, port);
}

void MyAPI::ServerThread(int port) {
    MYLOG_INFO("MyAPI: 准备启动 REST 环境...");
    oatpp::base::Environment::init();

    {
        auto objectMapper = std::make_shared<oatpp::parser::json::mapping::ObjectMapper>();
        std::string OATPP_SWAGGER_RES_PATH = "/home/cs/DockerRoot/fast_cpp_server/external/oatpp-swagger/res";
        if (MyINIConfig::GetInstance().HasKey("swagger_res_dir")) {
            std::string res_dir;
            MyINIConfig::GetInstance().GetString("swagger_res_dir", "", res_dir);
            if (res_dir.empty()) {
                MYLOG_WARN("MyAPI: 配置项 swagger_res_dir 为空，使用默认路径: {}", OATPP_SWAGGER_RES_PATH);
            }
            // 判断路径是否存在
            else {
                if (std::filesystem::exists(res_dir)) {
                    OATPP_SWAGGER_RES_PATH = res_dir;
                    MYLOG_INFO("MyAPI: 使用配置的 Swagger 资源路径: {}", OATPP_SWAGGER_RES_PATH);
                } else {
                    MYLOG_WARN("MyAPI: 配置的 swagger_res_dir 路径不存在，使用默认路径: {}", OATPP_SWAGGER_RES_PATH);
                }
            }
        }
        auto swaggerResources = oatpp::swagger::Resources::loadResources(OATPP_SWAGGER_RES_PATH);

        auto docInfo = oatpp::swagger::DocumentInfo::Builder()
            .setTitle("Fast C++ Server: API Server")
            .setVersion("1.0")
            .build();

        auto router = oatpp::web::server::HttpRouter::createShared();
        auto docEndpoints = oatpp::web::server::api::Endpoints();

        auto edgeController = my_api::edge::EdgeController::createShared(
            std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper)
        );
        router->addController(edgeController);
        docEndpoints.append(edgeController->getEndpoints());


        auto heartbeatController =
        my_api::heartbeat::HeartBeatController::createShared(
            std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper)
        );

        router->addController(heartbeatController);
        docEndpoints.append(heartbeatController->getEndpoints());

        auto edgesController = my_api::edge::EdgesController::createShared(
            std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper)
        );
        router->addController(edgesController);
        docEndpoints.append(edgesController->getEndpoints());

        auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints, docInfo, swaggerResources);
        router->addController(swaggerController);

        // --- 修正点：ConnectionHandler 只声明一次 ---
        auto connectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);
        
        // 添加请求拦截器 (处理 OPTIONS)
        connectionHandler->addRequestInterceptor(std::make_shared<CorsInterceptor>());
        // 添加响应拦截器 (处理所有请求的 Header)
        connectionHandler->addResponseInterceptor(std::make_shared<CorsResponseInterceptor>());

        auto connectionProvider = oatpp::network::tcp::server::ConnectionProvider::createShared(
            {"0.0.0.0", (v_uint16)port, oatpp::network::Address::IP_4});
        
        oatpp::network::Server server(connectionProvider, connectionHandler);

        MYLOG_INFO("MyAPI: REST Server 已就绪: http://127.0.0.1:{}/swagger/ui", port);
        server.run([this](){ return is_running_.load(); }); 
    }

    oatpp::base::Environment::destroy();
}

void MyAPI::Stop() {
    is_running_ = false;
    if (server_thread_.joinable()) server_thread_.join();
}

// 封装的启动单例函数
void RunRestServer(int port) {
    auto& api = MyAPI::GetInstance();
    std::signal(SIGINT, [](int) { MyAPI::GetInstance().Stop(); });
    api.Start(port);
    while (api.IsRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} // namespace my_api