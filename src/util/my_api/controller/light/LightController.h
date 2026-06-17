#pragma once

/**
 * @file LightController.h
 * @brief 探照灯模块 REST API 控制器
 *
 * 对外暴露以下接口（路由前缀 /v1/light）：
 *
 * LED 控制：
 * - POST /v1/light/led/open          : 开启 LED
 * - POST /v1/light/led/close         : 关闭 LED
 * - POST /v1/light/led/level         : 设置亮度（0～100）
 *
 * 频闪控制：
 * - POST /v1/light/flash/open        : 开启频闪模式
 * - POST /v1/light/flash/close       : 关闭频闪模式
 * - POST /v1/light/flash/setting     : 设置频闪频率（1～50 Hz）
 *
 * 云台控制：
 * - POST /v1/light/servo/home        : 云台复位回中
 * - POST /v1/light/servo/move_to     : 云台移动到绝对角度
 * - POST /v1/light/servo/move_dir    : 云台按方向移动（步长 5°）
 *
 * 状态查询：
 * - GET  /v1/light/status            : 查看探照灯当前状态
 */

#include "BaseApiController.hpp"
#include "dto/light/LightDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

namespace my_api::light_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class LightController : public base::BaseApiController {
public:
    explicit LightController(const std::shared_ptr<ObjectMapper>& objectMapper);

    static std::shared_ptr<LightController> createShared(
        const std::shared_ptr<ObjectMapper>& objectMapper);

    // ==================== LED 控制接口 ====================

    ENDPOINT_INFO(openLed) {
        info->summary     = "开启探照灯 LED";
        info->description = "向探照灯设备发送 LED 开启命令。设备离线时返回 503。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/led/open", openLed);

    ENDPOINT_INFO(closeLed) {
        info->summary     = "关闭探照灯 LED";
        info->description = "向探照灯设备发送 LED 关闭命令。设备离线时返回 503。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/led/close", closeLed);

    ENDPOINT_INFO(setLightLevel) {
        info->summary     = "设置 LED 亮度";
        info->description = "设置探照灯亮度级别，level 范围 0～100。超出范围返回 400，设备离线返回 503。";
        info->addConsumes<oatpp::Object<my_api::dto::LightLevelDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/led/level", setLightLevel,
             BODY_DTO(oatpp::Object<my_api::dto::LightLevelDto>, levelDto));

    // ==================== 频闪控制接口 ====================

    ENDPOINT_INFO(openFlash) {
        info->summary     = "开启频闪模式";
        info->description = "开启探照灯频闪模式。设备离线时返回 503。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/flash/open", openFlash);

    ENDPOINT_INFO(closeFlash) {
        info->summary     = "关闭频闪模式";
        info->description = "关闭探照灯频闪模式。设备离线时返回 503。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/flash/close", closeFlash);

    ENDPOINT_INFO(setFlash) {
        info->summary     = "设置频闪频率";
        info->description = "设置频闪频率，hz 范围 1～50。超出范围返回 400，设备离线返回 503。";
        info->addConsumes<oatpp::Object<my_api::dto::LightFlashHzDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/flash/setting", setFlash,
             BODY_DTO(oatpp::Object<my_api::dto::LightFlashHzDto>, flashDto));

    // ==================== 云台控制接口 ====================

    ENDPOINT_INFO(servoHome) {
        info->summary     = "云台复位";
        info->description = "控制探照灯云台回中到初始位置。设备离线时返回 503。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/servo/home", servoHome);

    ENDPOINT_INFO(servoMoveTo) {
        info->summary     = "云台移动到绝对角度";
        info->description = "控制探照灯云台移动到指定绝对角度，x(-180～180)，y(-180～180)。超出范围返回 400，设备离线返回 503。";
        info->addConsumes<oatpp::Object<my_api::dto::LightMoveToDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/servo/move_to", servoMoveTo,
             BODY_DTO(oatpp::Object<my_api::dto::LightMoveToDto>, moveDto));

    ENDPOINT_INFO(servMoveDir) {
        info->summary     = "云台按方向步进移动";
        info->description = "控制探照灯云台向指定方向移动固定步长（5°），direction：up/down/left/right。方向非法返回 400，设备离线返回 503。";
        info->addConsumes<oatpp::Object<my_api::dto::LightDirectionDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("POST", "/v1/light/servo/move_dir", servMoveDir,
             BODY_DTO(oatpp::Object<my_api::dto::LightDirectionDto>, dirDto));

    // ==================== 状态查询接口 ====================

    ENDPOINT_INFO(getStatus) {
        info->summary     = "查看探照灯结构化状态";
        info->description = "返回探照灯当前语义化状态：在线标志、LED 开关、频闪开关、亮度、频闪频率、云台角度。字段已归一化，便于前端直接使用。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/light/get_status", getStatus);

    ENDPOINT_INFO(getLightStatus) {
        info->summary     = "查看探照灯原始状态快照";
        info->description = "返回探照灯底层设备状态快照原始 JSON，包含帧计数、校验错误数等调试字段。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/light/status", getLightStatus);
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::light_api
