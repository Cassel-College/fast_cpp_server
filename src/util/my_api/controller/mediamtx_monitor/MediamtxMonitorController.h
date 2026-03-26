#pragma once

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::mediamtx_monitor_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class MediamtxMonitorController : public base::BaseApiController {
public:
    explicit MediamtxMonitorController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<MediamtxMonitorController>
    createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    // --- 在线检查 ---
    ENDPOINT_INFO(getOnline) {
        info->summary = "MediaMTX Monitor 存活检查";
        info->description = "检查 MediaMTX Monitor 服务是否存活。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/mediamtx/online", getOnline);

    // --- 获取状态 ---
    ENDPOINT_INFO(getStatus) {
        info->summary = "获取 MediaMTX Monitor 状态";
        info->description = "返回 MediaMTX Monitor 当前运行状态与心跳信息。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/mediamtx/status", getStatus);

    // --- 启动 ---
    ENDPOINT_INFO(postStart) {
        info->summary = "启动 MediaMTX Monitor";
        info->description = "启动 RTSP 转发守护监控线程。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("POST", "/v1/mediamtx/start", postStart);

    // --- 停止 ---
    ENDPOINT_INFO(postStop) {
        info->summary = "停止 MediaMTX Monitor";
        info->description = "停止 RTSP 转发守护监控线程并终止 MediaMTX 进程。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("POST", "/v1/mediamtx/stop", postStop);

    // --- 基本信息 ---
    ENDPOINT_INFO(getInfo) {
        info->summary = "获取 MediaMTX 转发代理基本信息";
        info->description = "返回 RTSP 源信息、本机信息、拉流方式及帮助信息等。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/mediamtx/info", getInfo);
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::mediamtx_monitor_api
