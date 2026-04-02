#pragma once

/**
 * @file AudioController.h
 * @brief 扬声器模块 REST API 控制器
 *
 * 对外暴露以下接口：
 * - GET  /v1/audio/config   : 查看启动参数（初始化配置）
 * - GET  /v1/audio/info     : 查看指定设备详细信息
 * - GET  /v1/audio/status   : 查看全部设备状态
 * - GET  /v1/audio/available: 查看当前可用扬声器列表
 * - POST /v1/audio/play     : 在指定设备上播放音频文件
 * - POST /v1/audio/volume   : 设置指定设备音量
 */

#include "BaseApiController.hpp"
#include "dto/audio/AudioDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

namespace my_api::audio_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class AudioController : public base::BaseApiController {
public:
    explicit AudioController(const std::shared_ptr<ObjectMapper>& objectMapper);

    static std::shared_ptr<AudioController> createShared(
        const std::shared_ptr<ObjectMapper>& objectMapper);

    // ==================== 查询类接口 ====================

    ENDPOINT_INFO(getAudioConfig) {
        info->summary = "查看扬声器启动参数";
        info->description = "返回音频设备管理器初始化时使用的 JSON 配置参数。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("GET", "/v1/audio/config", getAudioConfig);

    ENDPOINT_INFO(getAudioInfo) {
        info->summary = "查看指定扬声器详细信息";
        info->description = "根据 body 中的 device_id 获取单个扬声器的配置、状态和设备信息。";
        info->addConsumes<oatpp::Object<my_api::dto::AudioDeviceIdDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/audio/info", getAudioInfo,
             BODY_DTO(oatpp::Object<my_api::dto::AudioDeviceIdDto>, deviceDto));

    ENDPOINT_INFO(getAudioStatus) {
        info->summary = "查看全部扬声器状态";
        info->description = "返回所有已注册扬声器的状态综合视图，包括名称、状态、配置和设备信息。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("GET", "/v1/audio/status", getAudioStatus);

    ENDPOINT_INFO(getAvailableSpeakers) {
        info->summary = "查看可用扬声器列表";
        info->description = "返回当前状态为空闲或工作中的扬声器 ID 列表。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_503, "application/json");
    }
    ENDPOINT("GET", "/v1/audio/available", getAvailableSpeakers);

    // ==================== 控制类接口 ====================

    ENDPOINT_INFO(playAudioFile) {
        info->summary = "在指定扬声器上播放音频文件";
        info->description = "指定 device_id 和 filepath，在目标扬声器上播放音频文件。";
        info->addConsumes<oatpp::Object<my_api::dto::AudioPlayRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/audio/play", playAudioFile,
             BODY_DTO(oatpp::Object<my_api::dto::AudioPlayRequestDto>, playDto));

    ENDPOINT_INFO(setVolume) {
        info->summary = "设置指定扬声器音量";
        info->description = "指定 device_id 和 volume（0-100），设置目标扬声器的音量。";
        info->addConsumes<oatpp::Object<my_api::dto::AudioVolumeRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/audio/volume", setVolume,
             BODY_DTO(oatpp::Object<my_api::dto::AudioVolumeRequestDto>, volumeDto));
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::audio_api
