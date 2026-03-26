#include "MyAPI.h"


#include "controller/heartbeat_manager/HeartBeatController.h"
#include "controller/script/ScriptController.h"
#include "controller/soft_healthy/SoftHealthyController.h"
#include "controller/fly_control/FlyControlController.h"
#include "controller/pod/PodController.h"
#include "controller/mediamtx_monitor/MediamtxMonitorController.h"

#include "controller/demo/edges/EdgesController.hpp"
#include "controller/demo/edge_manager/EdgeController.hpp"
#include "controller/demo/tuna/TunaController.h"

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

#include "BaseApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include "MyINIConfig.h"
#include "MyLog.h"
#include <sys/stat.h>
#include <vector>


#include OATPP_CODEGEN_BEGIN(ApiController)

namespace my_api {

namespace {

constexpr const char* kDefaultSwaggerResourceDir = "/home/cs/DockerRoot/fast_cpp_server/external/oatpp-swagger/res";

class OatppEnvironmentGuard {
public:
    OatppEnvironmentGuard() {
        oatpp::base::Environment::init();
    }

    ~OatppEnvironmentGuard() {
        oatpp::base::Environment::destroy();
    }
};

bool DirectoryExists(const std::string& directory) {
    if (directory.empty()) {
        return false;
    }

    struct stat info {};
    return ::stat(directory.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

std::string ResolveSwaggerResourceDir() {
    const std::string default_dir = kDefaultSwaggerResourceDir;

    if (!MyINIConfig::IsInitialized()) {
        MYLOG_WARN("MyAPI: INI 配置尚未初始化，使用默认 Swagger 资源路径: {}", default_dir);
        return default_dir;
    }

    try {
        std::string configured_dir;
        MyINIConfig::GetInstance().GetString("swagger_res_dir", "", configured_dir);

        if (configured_dir.empty()) {
            MYLOG_WARN("MyAPI: 配置项 swagger_res_dir 为空，使用默认路径: {}", default_dir);
            return default_dir;
        }

        if (DirectoryExists(configured_dir)) {
            MYLOG_INFO("MyAPI: 使用配置的 Swagger 资源路径: {}", configured_dir);
            return configured_dir;
        }

        MYLOG_WARN("MyAPI: 配置的 swagger_res_dir 不存在或不是目录，使用默认路径: {}", default_dir);
    } catch (const std::exception& e) {
        MYLOG_WARN("MyAPI: 读取 swagger_res_dir 失败，使用默认路径: {}，error={}", default_dir, e.what());
    } catch (...) {
        MYLOG_WARN("MyAPI: 读取 swagger_res_dir 失败，使用默认路径: {}，error=unknown", default_dir);
    }

    return default_dir;
}

std::shared_ptr<oatpp::swagger::Resources> LoadSwaggerResources() {
    const std::string default_dir = kDefaultSwaggerResourceDir;
    const std::string selected_dir = ResolveSwaggerResourceDir();

    try {
        return oatpp::swagger::Resources::loadResources(selected_dir);
    } catch (const std::exception& e) {
        MYLOG_WARN("MyAPI: 加载 Swagger 资源失败，path={}, error={}", selected_dir, e.what());
    } catch (...) {
        MYLOG_WARN("MyAPI: 加载 Swagger 资源失败，path={}, error=unknown", selected_dir);
    }

    if (selected_dir != default_dir) {
        try {
            MYLOG_WARN("MyAPI: 尝试回退到默认 Swagger 资源路径: {}", default_dir);
            return oatpp::swagger::Resources::loadResources(default_dir);
        } catch (const std::exception& e) {
            MYLOG_ERROR("MyAPI: 默认 Swagger 资源加载失败，path={}, error={}", default_dir, e.what());
        } catch (...) {
            MYLOG_ERROR("MyAPI: 默认 Swagger 资源加载失败，path={}, error=unknown", default_dir);
        }
    }

    return nullptr;
}

} // namespace

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
    OatppEnvironmentGuard environment_guard;

    try {
        auto objectMapper = std::make_shared<oatpp::parser::json::mapping::ObjectMapper>();
        auto swaggerResources = LoadSwaggerResources();

        auto docInfo = oatpp::swagger::DocumentInfo::Builder()
            .setTitle("Fast C++ Server: API Server")
            .setVersion("1.0")
            .build();

        std::vector<std::shared_ptr<base::BaseApiController>> objectMappers; // 如果不同控制器需要不同的 ObjectMapper，可以在这里创建并传递

        auto router = oatpp::web::server::HttpRouter::createShared();
        auto docEndpoints = oatpp::web::server::api::Endpoints();

        auto edgeController = my_api::edge_manager::EdgeController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(edgeController);
        docEndpoints.append(edgeController->getEndpoints());

        auto heartbeatController = my_api::heartbeat::HeartBeatController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(heartbeatController);
        docEndpoints.append(heartbeatController->getEndpoints());

        auto edgesController = my_api::edge::EdgesController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(edgesController);
        docEndpoints.append(edgesController->getEndpoints());

        auto scriptController = my_api::my_script_api::MyScriptController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(scriptController);
        docEndpoints.append(scriptController->getEndpoints());

        auto tunaController = my_api::tuna::TunaController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(tunaController);
        docEndpoints.append(tunaController->getEndpoints());

        auto softHealthyController = my_api::soft_healthy::SoftHealthyController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(softHealthyController);
        docEndpoints.append(softHealthyController->getEndpoints());

        auto flyControlController = my_api::fly_control_api::FlyControlController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(flyControlController);
        docEndpoints.append(flyControlController->getEndpoints());

        auto podController = my_api::pod_api::PodController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(podController);
        docEndpoints.append(podController->getEndpoints());

        auto mediamtxController = my_api::mediamtx_monitor_api::MediamtxMonitorController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        router->addController(mediamtxController);
        docEndpoints.append(mediamtxController->getEndpoints());

        if (swaggerResources != nullptr) {
            auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints, docInfo, swaggerResources);
            router->addController(swaggerController);
        } else {
            MYLOG_WARN("MyAPI: Swagger 资源不可用，跳过 Swagger Controller 注册");
        }
        // docEndpoints.append(swaggerController->getEndpoints());

        // using MyobjectMapper = oatpp::data::mapping::ObjectMapper;
        // auto edgeController = my_api::edge_manager::EdgeController::createShared(std::static_pointer_cast<MyobjectMapper>(objectMapper));
        // auto heartbeatController = my_api::heartbeat::HeartBeatController::createShared(std::static_pointer_cast<MyobjectMapper>(objectMapper));
        // auto edgesController = my_api::edge::EdgesController::createShared(std::static_pointer_cast<MyobjectMapper>(objectMapper));
        // auto scriptController = my_api::my_script_api::MyScriptController::createShared(std::static_pointer_cast<MyobjectMapper>(objectMapper));
        // auto tunaController = my_api::tuna::TunaController::createShared(std::static_pointer_cast<MyobjectMapper>(objectMapper));
        // auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints, docInfo, swaggerResources);
        // objectMappers.push_back(edgeController);
        // objectMappers.push_back(heartbeatController);
        // objectMappers.push_back(edgesController);
        // objectMappers.push_back(scriptController);
        // objectMappers.push_back(tunaController);

        // for (std::shared_ptr<base::BaseApiController> objectMapper : objectMappers) {
        //     router->addController(objectMapper);
        //     docEndpoints.append(objectMapper->getEndpoints());
        // }
        // router->addController(swaggerController);

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
    } catch (const std::exception& e) {
        MYLOG_ERROR("MyAPI: REST 线程启动失败: {}", e.what());
    } catch (...) {
        MYLOG_ERROR("MyAPI: REST 线程启动失败: unknown exception");
    }

    is_running_.store(false);
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