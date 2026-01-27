#pragma once

#include <nlohmann/json.hpp>
// #include "nlohmann/json_fwd.hpp"
// #include "oatpp-swagger/oas3/Model.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

class HeartbeatDto : public oatpp::DTO {
    DTO_INIT(HeartbeatDto, DTO)

    DTO_FIELD(String,           from);        // 心跳来源
    DTO_FIELD(Int64,            timestamp);   // 时间戳
    DTO_FIELD(String,           status);      // 状态
    // DTO_FIELD(nlohmann::json,   heartbeat);   // 额外心跳数据
    DTO_FIELD(String,           heartbeat);   // 额外心跳数据（JSON 字符串）
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto
