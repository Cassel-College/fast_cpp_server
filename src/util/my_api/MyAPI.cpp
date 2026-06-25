#include "MyAPI.h"


#include "controller/heartbeat_manager/HeartBeatController.h"
#include "controller/script/ScriptController.h"
#include "controller/soft_healthy/SoftHealthyController.h"
#include "controller/fly_control/FlyControlController.h"
#include "controller/pod/PodController.h"
#include "controller/mediamtx_monitor/MediamtxMonitorController.h"
#include "controller/file_cache/FileApiController.h"

#include "controller/audio/AudioController.h"
#include "controller/light/LightController.h"
#include "controller/ip/MyIPController.h"
#include "controller/demo/edges/EdgesController.hpp"
#include "controller/demo/tuna/TunaController.h"
#include "controller/context/ContextController.h"

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

constexpr const char* kDefaultSwaggerResourceDir = "/opt/fast_cpp_server/share/swagger-res/res";

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

/**
 * @brief 从传入的 JSON 配置中生成启动参数配置文件, 并保存到 api_enable_mapping_ 成员变量中, 主要是解析 "executes" 字段下的每个节点，提取 "model_name" 和 "enable" 字段。
 * 
 * @param pipeline_config JSON 配置对象
 */
void MyAPI::GenerateStartSettingByPipelineConfig(const nlohmann::json& pipeline_config) {
    try {
        if (!pipeline_config.is_object()) {
            MYLOG_WARN("MyAPI: pipeline_config 不是对象，忽略本次更新");
            return;
        }

        if (!pipeline_config.contains("executes")) {
            MYLOG_WARN("MyAPI: pipeline_config 缺少 executes 字段，忽略本次更新");
            return;
        }

        const auto& executes = pipeline_config.at("executes");
        if (!executes.is_object() && !executes.is_array()) {
            MYLOG_WARN("MyAPI: executes 字段类型错误，期望 object 或 array，实际类型={}", executes.type_name());
            return;
        }

        nlohmann::json next_mapping = nlohmann::json::object();
        std::size_t valid_count = 0;
        std::size_t skipped_count = 0;

        auto register_model = [&](const std::string& node_key, const nlohmann::json& node_body) {
            if (!node_body.is_object()) {
                ++skipped_count;
                MYLOG_WARN("MyAPI: executes[{}] 不是对象，已跳过", node_key);
                return;
            }

            if (!node_body.contains("model_name") || !node_body["model_name"].is_string()) {
                ++skipped_count;
                MYLOG_WARN("MyAPI: executes[{}] 缺少 model_name 字段或类型错误，已跳过", node_key);
                return;
            }

            const std::string model_name = node_body["model_name"].get<std::string>();
            if (model_name.empty()) {
                ++skipped_count;
                MYLOG_WARN("MyAPI: executes[{}] 的 model_name 为空，已跳过", node_key);
                return;
            }

            bool enable = true;
            if (node_body.contains("enable")) {
                if (!node_body["enable"].is_boolean()) {
                    ++skipped_count;
                    MYLOG_WARN("MyAPI: executes[{}].enable 类型错误，已跳过模型 {}", node_key, model_name);
                    return;
                }
                enable = node_body["enable"].get<bool>();
            } else {
                MYLOG_WARN("MyAPI: executes[{}] 缺少 enable 字段，模型 {} 默认按 true 处理", node_key, model_name);
            }

            auto inserted = next_mapping.emplace(model_name, enable);
            if (!inserted.second) {
                MYLOG_WARN("MyAPI: 模型 {} 重复出现，已覆盖旧值 -> {}", model_name, enable);
                inserted.first.value() = enable;
            }
            ++valid_count;

            MYLOG_INFO("MyAPI: 解析启动项成功, node={}, model_name={}, enable={}", node_key, model_name, enable);
        };

        if (executes.is_object()) {
            for (const auto& [key, value] : executes.items()) {
                register_model(key, value);
            }
        } else {
            for (std::size_t index = 0; index < executes.size(); ++index) {
                register_model(std::to_string(index), executes.at(index));
            }
        }

        api_enable_mapping_ = std::move(next_mapping);

        MYLOG_INFO(
            "MyAPI: 生成启动参数配置文件成功, \nvalid_count={}, \nskipped_count={}, \napi_enable_mapping_={}",
            valid_count,
            skipped_count,
            api_enable_mapping_.dump(4)
        );
    } catch (const std::exception& e) {
        MYLOG_ERROR("MyAPI: 生成启动参数配置文件失败, error={}", e.what());
    } catch (...) {
        MYLOG_ERROR("MyAPI: 生成启动参数配置文件失败, error=unknown");
    }
}

bool MyAPI::getJsonBool(const nlohmann::json& j, const std::string& key, bool defaultValue) {
    try {
        if (!j.is_object())
        {
            return defaultValue;
        }

        auto it = j.find(key);
        if (it == j.end())
        {
            return defaultValue;
        }

        if (!it->is_boolean())
        {
            return defaultValue;
        }

        return it->get<bool>();
    }
    catch (...)
    {
        return defaultValue;
    }
}

void MyAPI::Start(int port) {
    if (is_running_.exchange(true)) return;
    server_thread_ = std::thread(&MyAPI::ServerThread, this, port);
}

bool MyAPI::LoadAPIModel(
    std::shared_ptr<oatpp::web::server::HttpRouter> router,
    oatpp::web::server::api::Endpoints &docEndpoints, 
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> objectMapper,
    std::string model_name) {

    bool load_status = false;
    bool has_model = false;

    for (const auto& loaded_model : loaded_models_) {
        if (loaded_model == model_name) {
            MYLOG_WARN("MyAPI: 模型 {} 已经加载过，跳过重复加载", model_name);
            return true; // 已经加载过，直接返回成功
        }
    }

    auto controller = std::shared_ptr<base::BaseApiController>(nullptr);
    if ("heartbeat" == model_name) {
        MYLOG_INFO("MyAPI: 加载心跳 API 模型");
        controller = my_api::heartbeat::HeartBeatController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("edges" == model_name) {
        MYLOG_INFO("MyAPI: 加载边缘体 API 模型");
        controller = my_api::edge::EdgesController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("script" == model_name) {
        MYLOG_INFO("MyAPI: 加载脚本 API 模型");
        controller = my_api::my_script_api::MyScriptController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("tuna" == model_name) {
        MYLOG_INFO("MyAPI: 加载 Tuna API 模型");
        controller = my_api::tuna::TunaController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("soft_healthy" == model_name) {
        MYLOG_INFO("MyAPI: 加载软件健康 API 模型");
        controller = my_api::soft_healthy::SoftHealthyController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("fly_control" == model_name) {
        MYLOG_INFO("MyAPI: 加载飞控 API 模型");
        controller = my_api::fly_control_api::FlyControlController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("pod" == model_name) {
        MYLOG_INFO("MyAPI: 加载 Pod API 模型");
        controller = my_api::pod_api::PodController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("mediamtx_monitor" == model_name) {
        MYLOG_INFO("MyAPI: 加载 Mediamtx Monitor API 模型");
        controller = my_api::mediamtx_monitor_api::MediamtxMonitorController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("file_cache" == model_name) {
        MYLOG_INFO("MyAPI: 加载 File Cache API 模型");
        controller = my_api::file_cache_api::FileApiController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("audio_server" == model_name) {
        MYLOG_INFO("MyAPI: 加载 Audio Server API 模型");
        controller = my_api::audio_api::AudioController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("search_light" == model_name) {
        MYLOG_INFO("MyAPI: 加载 Search Light API 模型");
        controller = my_api::light_api::LightController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("ip" == model_name) {
        MYLOG_INFO("MyAPI: 加载 IP API 模型");
        controller = my_api::ip_api::MyIPController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else if ("context" == model_name) {
        MYLOG_INFO("MyAPI: 加载运行时上下文 API 模型");
        controller = my_api::context_api::ContextController::createShared(std::static_pointer_cast<oatpp::data::mapping::ObjectMapper>(objectMapper));
        has_model = true;
    } else {
        MYLOG_WARN("MyAPI: 未知的 API 模型名称: {}", model_name);
    }

    if (has_model) {
        if(getJsonBool(api_enable_mapping_, model_name, true)) {
            router->addController(controller);
            docEndpoints.append(controller->getEndpoints());
            load_status = true;
            loaded_models_.push_back(model_name); // 记录已加载的模型
            MYLOG_INFO("MyAPI: {} API 控制器加载成功", model_name.c_str());
            MYLOG_INFO("MyAPI: 当前已加载的 API 模型列表新增: {}， 现在加载的模型数量: {}", model_name.c_str(), loaded_models_.size());
        } else {
            MYLOG_INFO("MyAPI: {} API 控制器已禁用，跳过注册", model_name.c_str());
        }
    } else {
        MYLOG_WARN("MyAPI: 未找到对应的 API 模型: {}", model_name);
    }
    return load_status;
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

        std::shared_ptr<oatpp::web::server::HttpRouter> router = oatpp::web::server::HttpRouter::createShared();
        auto docEndpoints = oatpp::web::server::api::Endpoints();

        // 加载默认 API 控制器
        std::vector<std::string> default_models = {
            "heartbeat", 
            "script",
            "soft_healthy",
            "file_cache",
            "ip",
            "context"
        };
        for (const auto& model_name : default_models) {
            if (LoadAPIModel(router, docEndpoints, objectMapper, model_name)) {
                MYLOG_INFO("MyAPI: {} API 控制器已启用并加载成功", model_name.c_str());
            } else {
                MYLOG_ERROR("MyAPI: {} API 控制器未启用或加载失败，跳过注册", model_name.c_str());
            }
        }

        // 加载功能性 API 控制器
        for (const auto& [model_name, enabled] : api_enable_mapping_.items()) {
            if (enabled && std::find(default_models.begin(), default_models.end(), model_name) == default_models.end()) {
                if (LoadAPIModel(router, docEndpoints, objectMapper, model_name)) {
                    MYLOG_INFO("MyAPI: {} API 控制器已启用并加载成功", model_name.c_str());
                } else {
                    MYLOG_ERROR("MyAPI: {} API 控制器未启用或加载失败，跳过注册", model_name.c_str());
                }
            }
        }

        if (swaggerResources != nullptr) {
            auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints, docInfo, swaggerResources);
            router->addController(swaggerController);
        } else {
            MYLOG_WARN("MyAPI: Swagger 资源不可用，跳过 Swagger Controller 注册");
        }

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