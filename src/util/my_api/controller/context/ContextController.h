#pragma once

/**
 * @file ContextController.h
 * @brief 运行时上下文（MyContext）REST API 控制器
 *
 * 对外暴露以下接口（路由前缀 /v1/context）：
 * - GET  /v1/context/online   : 服务存活检查
 * - GET  /v1/context/all      : 获取所有运行时上下文
 * - POST /v1/context/get      : 查看某个上下文
 * - POST /v1/context/set      : 设置上下文（不存在则创建，存在则覆盖，upsert）
 * - POST /v1/context/update   : 修改某个已存在的上下文的值
 * - POST /v1/context/delete   : 删除某个上下文
 */

#include "BaseApiController.hpp"
#include "dto/context/ContextDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

namespace my_api::context_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class ContextController : public base::BaseApiController {
public:
    static constexpr const char* SWAGGER_TAG = "ContextController";

    explicit ContextController(const std::shared_ptr<ObjectMapper>& objectMapper);

    static std::shared_ptr<ContextController> createShared(
        const std::shared_ptr<ObjectMapper>& objectMapper);

    // ==================== 存活检查 ====================

    ENDPOINT_INFO(getOnline) {
        info->addTag(SWAGGER_TAG);
        info->summary     = "上下文服务存活检查";
        info->description = "返回上下文 API 是否在线。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/context/online", getOnline);

    // ==================== 获取所有上下文 ====================

    ENDPOINT_INFO(getAllContext) {
        info->addTag(SWAGGER_TAG);
        info->summary     = "获取所有运行时上下文";
        info->description = "返回当前进程内全部上下文记录，格式为 { count, items: [ {name,value,description,type,...} ] }。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/v1/context/all", getAllContext);

    // ==================== 查看某个上下文 ====================

    ENDPOINT_INFO(getContext) {
        info->addTag(SWAGGER_TAG);
        info->summary     = "查看某个上下文";
        info->description = "根据变量名查询单条上下文记录。请求体示例：{ \"name\": \"is_recording\" }。不存在返回 404。";
        info->addConsumes<oatpp::Object<my_api::dto::ContextNameRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/context/get", getContext,
             BODY_DTO(oatpp::Object<my_api::dto::ContextNameRequestDto>, requestDto));

    // ==================== 设置（upsert）上下文 ====================

    ENDPOINT_INFO(setContext) {
        info->addTag(SWAGGER_TAG);
        info->summary     = "设置上下文（不存在则创建，存在则覆盖）";
        info->description =
            "设置一条上下文记录。value 可为 bool / 数值 / 字符串 / 对象 / 数组。"
            "请求体示例：{ \"name\": \"is_recording\", \"value\": true, \"description\": \"是否正在录制\" }。"
            "description 可选；省略时若记录已存在则保留原解释。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
    }
    ENDPOINT("POST", "/v1/context/set", setContext, BODY_STRING(oatpp::String, body));

    // ==================== 修改某个已存在的上下文 ====================

    ENDPOINT_INFO(updateContext) {
        info->addTag(SWAGGER_TAG);
        info->summary     = "修改某个上下文的值";
        info->description =
            "修改一条已存在的上下文记录的值（记录必须已存在，否则返回 404）。"
            "请求体示例：{ \"name\": \"is_recording\", \"value\": false }。"
            "可附带 description 字段以同步更新解释。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/context/update", updateContext, BODY_STRING(oatpp::String, body));

    // ==================== 删除某个上下文 ====================

    ENDPOINT_INFO(deleteContext) {
        info->addTag(SWAGGER_TAG);
        info->summary     = "删除某个上下文";
        info->description = "根据变量名删除单条上下文记录。请求体示例：{ \"name\": \"is_recording\" }。不存在返回 404。";
        info->addConsumes<oatpp::Object<my_api::dto::ContextNameRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST", "/v1/context/delete", deleteContext,
             BODY_DTO(oatpp::Object<my_api::dto::ContextNameRequestDto>, requestDto));
};

#include OATPP_CODEGEN_END(ApiController)

}  // namespace my_api::context_api
