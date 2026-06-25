#pragma once

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "dto/HeartbeatDto.hpp"

namespace my_api::heartbeat {

#include OATPP_CODEGEN_BEGIN(ApiController)

class HeartBeatController : public base::BaseApiController {
public:
    static constexpr const char* SWAGGER_TAG = "HeartBeatController";
    explicit HeartBeatController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<HeartBeatController>
    createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT_INFO(getHeartbeatOnline) {
        info->addTag(SWAGGER_TAG);
        info->summary = "心跳检查接口";
        info->description = "用于检查服务是否存活，返回简单的状态信息。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/heartbeat/online", getHeartbeatOnline);

    ENDPOINT_INFO(getHeartbeatData) {
        info->addTag(SWAGGER_TAG);
        info->summary = "获取心跳数据";
        info->description = "根据请求参数获取系统当前的心跳数据，包含系统状态、边缘设备状态等信息。";
        // info->addConsumes<oatpp::String>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
    }
    ENDPOINT(
        "GET", 
        "/v1/heartbeat/getHeartbeatJsonData", 
        getHeartbeatData
    );

    // 获取心跳启动参数
    ENDPOINT("GET", "/v1/heartbeat/config", getHeartbeatConfig);
             
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::heartbeat
