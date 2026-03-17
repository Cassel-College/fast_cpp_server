#pragma once

#include "BaseApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

namespace my_api::fly_control_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class FlyControlController : public base::BaseApiController {
public:
    explicit FlyControlController(const std::shared_ptr<ObjectMapper>& objectMapper);

    static std::shared_ptr<FlyControlController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT_INFO(getFlyControlOnline) {
        info->summary = "飞控 API 在线检查";
        info->description = "检查飞控 API 控制器是否可访问。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/flycontrol/online", getFlyControlOnline);

    ENDPOINT_INFO(getFlyControlStatus) {
        info->summary = "获取飞控状态与数据";
        info->description = "返回飞控管理器初始化状态、运行状态、是否收到心跳，以及最近一次飞控心跳数据。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/flycontrol/status", getFlyControlStatus);

    ENDPOINT_INFO(getFlyControlHeartbeatData) {
        info->summary = "获取最新飞控心跳 JSON";
        info->description = "返回最新飞控心跳数据。若尚未收到心跳，会返回 has_heartbeat=false。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/flycontrol/getHeartbeatJsonData", getFlyControlHeartbeatData);
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::fly_control_api