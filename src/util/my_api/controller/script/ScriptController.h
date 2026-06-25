#pragma once

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"


namespace my_api::my_script_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class MyScriptController : public base::BaseApiController {
public:
    static constexpr const char* SWAGGER_TAG = "ScriptController";
    explicit MyScriptController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<MyScriptController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT_INFO(getCurrentPythonInfo) {
        info->addTag(SWAGGER_TAG);
        info->summary = "获取当前系统 Python 环境信息";
        info->description = "返回当前系统 Python 环境的信息，包含可执行文件路径、版本、平台等。";
        info->addResponse<String>(Status::CODE_200, "application/json");
        info->addResponse<String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("GET", "/v1/script/getCurrentPythonInfo", getCurrentPythonInfo, REQUEST(std::shared_ptr<IncomingRequest>, request));
        
    
    // 远程执行python脚本，并返回执行结果
    ENDPOINT_INFO(runPythonScript) {
        info->addTag(SWAGGER_TAG);
        info->summary = "远程执行 Python 脚本";
        info->description = "在服务器上远程执行指定的 Python 脚本，并返回执行结果。";
        info->addResponse<String>(Status::CODE_200, "application/json");
        info->addResponse<String>(Status::CODE_500, "application/json");
    }
    // 接受任意未结构化数据（如原始 JSON 字符串）而不是强类型 DTO
    ENDPOINT("POST", "/v1/script/runPythonScript", runPythonScript, BODY_STRING(oatpp::String, body));
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::my_script_api
