#pragma once

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "dto/HeartbeatDto.hpp"

namespace my_api::heartbeat {

#include OATPP_CODEGEN_BEGIN(ApiController)

class HeartBeatController : public base::BaseApiController {
public:
    explicit HeartBeatController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<HeartBeatController>
    createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    // 服务存活检查
    ENDPOINT("GET", "/v1/heartbeat", getHeartbeat);

    // 心跳上报
    ENDPOINT("POST", "/v1/heartbeat", postHeartbeat,
             BODY_DTO(oatpp::Object<my_api::dto::HeartbeatDto>, heartbeat));

             
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::heartbeat
