#pragma once

/**
 * @file ContextDto.hpp
 * @brief 运行时上下文（MyContext）REST API 的数据传输对象（DTO）定义
 *
 * 注意：上下文的 value 可为任意类型（bool / 数值 / 字符串 / 对象 / 数组），
 * 因此「设置 / 修改」类接口直接以原始 JSON 请求体（BODY_STRING）传入，
 * 在控制器中使用 nlohmann::json 解析，本文件仅声明可静态描述的请求体。
 */

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * @brief 仅包含变量名的请求体（用于「查看某个」「删除某个」上下文）
 *
 * 示例 JSON:
 * {
 *   "name": "is_recording"
 * }
 */
class ContextNameRequestDto : public oatpp::DTO {
    DTO_INIT(ContextNameRequestDto, DTO)

    DTO_FIELD(String, name);
    DTO_FIELD_INFO(name) {
        info->description = "上下文变量名（唯一键）";
        info->required = true;
    }
};

#include OATPP_CODEGEN_END(DTO)

}  // namespace my_api::dto
