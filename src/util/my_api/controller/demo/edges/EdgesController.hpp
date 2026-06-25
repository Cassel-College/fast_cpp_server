#pragma once

#include "BaseApiController.hpp"
#include "dto/demo/EdgeStatusDto.hpp"
#include "dto/demo/TaskDto.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"



namespace my_api::edge {

#include OATPP_CODEGEN_BEGIN(ApiController)

class EdgesController : public base::BaseApiController {
public:
  static constexpr const char* SWAGGER_TAG = "EdgesController";
    explicit EdgesController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<EdgesController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT("GET", "/v1/edges/status", getEdgesStatus);

    ENDPOINT("POST", "/v1/edges/startAllEdges", startAllEdges);

    // ENDPOINT_INFO 提供 Swagger 示例与 response 类型（返回 JSON 字符串）
    ENDPOINT_INFO(appendTaskToEdgeById) {
      info->addTag(SWAGGER_TAG);
      info->summary = "向指定的 Edge 添加任务";
      info->description = "请求体示例:\n"
                          "{\n"
                          "  \"edgeId\": \"edge-001\",\n"
                          "  \"taskType\": \"run_job\",\n"
                          "  \"paramsJson\": \"{ \\\"cmd\\\": \\\"/usr/bin/do_something\\\", \\\"timeout\\\": 30 }\"\n"
                          "}\n"
                          "注意：paramsJson 是一个 JSON 字符串，业务端会把它解析为 JSON 对象。";
      info->addConsumes<oatpp::Object<my_api::dto::TaskDto>>("application/json");
      // 返回原始 JSON 字符串（统一封装后的响应），使用 oatpp::String 类型
      info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
      info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
      info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST",
             "/v1/edges/appendTaskToEdgeById",
             appendTaskToEdgeById,
             BODY_DTO(oatpp::Object<my_api::dto::TaskDto>, taskDto)
            );

    ENDPOINT_INFO(getOnlineEdges) {
      info->addTag(SWAGGER_TAG);
      info->summary = "获取所有在线的 Edge ID 列表";
      info->description = "返回在线的 Edge 设备 ID 列表。";
      info->addResponse<oatpp::Vector<oatpp::String>>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET",
             "/v1/edges/getOnlineEdges",
             getOnlineEdges
            );

    ENDPOINT_INFO(getTargetEdgeDumpInternalInfo) {
      info->addTag(SWAGGER_TAG);
        info->summary = "获取指定 Edge 的内部信息 Dump";
        info->description = "返回指定 Edge 的内部信息，用于调试和诊断。";
        info->addConsumes<oatpp::Object<my_api::dto::EdgeIDDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST",
             "/v1/edges/dumpInternalInfo",
             getTargetEdgeDumpInternalInfo,
             BODY_DTO(oatpp::Object<my_api::dto::EdgeIDDto>, edgeIdDto)
            );

    ENDPOINT_INFO(getTargetEdgeRunTimeStatusInfo) {
      info->addTag(SWAGGER_TAG);
        info->summary = "获取指定 Edge 的运行时状态信息";
        info->description = "返回指定 Edge 的运行时状态信息，用于调试和诊断。";
        info->addConsumes<oatpp::Object<my_api::dto::EdgeIDDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST",
             "/v1/edges/getTargetEdgeRunTimeStatusInfo",
             getTargetEdgeRunTimeStatusInfo,
             BODY_DTO(oatpp::Object<my_api::dto::EdgeIDDto>, edgeIdDto)
            );

}; // class EdgesController
#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::edge