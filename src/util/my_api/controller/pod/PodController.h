#pragma once

#include "BaseApiController.hpp"
#include "dto/pod/PodDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

namespace my_api::pod_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class PodController : public base::BaseApiController {
public:
    explicit PodController(const std::shared_ptr<ObjectMapper>& objectMapper);

    static std::shared_ptr<PodController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    // ==================== 查询类接口 ====================

    ENDPOINT_INFO(getPodOnline) {
        info->summary = "吊舱模块在线检查";
        info->description = "检查吊舱管理模块是否已初始化并可用。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/pod/online", getPodOnline);

    ENDPOINT_INFO(getPodStatus) {
        info->summary = "获取所有吊舱状态";
        info->description = "返回所有已注册吊舱的状态摘要，包括连接状态、在线状态等。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/pod/status", getPodStatus);

    ENDPOINT_INFO(getPodDetail) {
        info->summary = "获取指定吊舱详情";
        info->description = "根据 body 中的 pod_id 获取单个吊舱的详细信息。";
        info->addConsumes<oatpp::Object<my_api::dto::PodIdDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/pod/detail", getPodDetail, BODY_DTO(oatpp::Object<my_api::dto::PodIdDto>, podDto));

    ENDPOINT_INFO(getPodConfig) {
        info->summary = "获取吊舱模块初始化配置";
        info->description = "返回吊舱管理器初始化时使用的 JSON 配置。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/pod/config", getPodConfig);

    ENDPOINT_INFO(getPodPtzPose) {
        info->summary = "获取指定吊舱云台姿态";
        info->description = "通过 JSON Body 中的 pod_id 获取指定吊舱当前云台姿态。";
        info->addConsumes<oatpp::Object<my_api::dto::PodPtzPoseQueryDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/pod/ptz/pose", getPodPtzPose,
             BODY_DTO(oatpp::Object<my_api::dto::PodPtzPoseQueryDto>, poseQueryDto));

    // ==================== 控制类接口 ====================

    ENDPOINT_INFO(connectPod) {
        info->summary = "连接指定吊舱";
        info->description = "通过 body 中的 pod_id 对吊舱发起连接。";
        info->addConsumes<oatpp::Object<my_api::dto::PodIdDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/pod/connect", connectPod, BODY_DTO(oatpp::Object<my_api::dto::PodIdDto>, podDto));

    ENDPOINT_INFO(disconnectPod) {
        info->summary = "断开指定吊舱";
        info->description = "通过 body 中的 pod_id 对吊舱执行断开操作。";
        info->addConsumes<oatpp::Object<my_api::dto::PodIdDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/pod/disconnect", disconnectPod, BODY_DTO(oatpp::Object<my_api::dto::PodIdDto>, podDto));

    ENDPOINT_INFO(controlPodPtz) {
        info->summary = "发送吊舱 PTZ 控制指令";
        info->description = "通过 JSON Body 中的 pod_id、action、step 发送 wsadh 模式云台控制指令。";
        info->addConsumes<oatpp::Object<my_api::dto::PodPtzControlDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/pod/ptz/control", controlPodPtz,
             BODY_DTO(oatpp::Object<my_api::dto::PodPtzControlDto>, controlDto));

    ENDPOINT_INFO(listPodIds) {
        info->summary = "列出所有吊舱ID";
        info->description = "返回当前已注册的所有吊舱 ID 列表。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/pod/list", listPodIds);
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::pod_api
