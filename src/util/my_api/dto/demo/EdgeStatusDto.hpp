#pragma once

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

class EdgeStatusDto : public oatpp::DTO {
    DTO_INIT(EdgeStatusDto, DTO)

    DTO_FIELD(String,   name);
    DTO_FIELD(String,   ip);
    DTO_FIELD(Boolean,  online);
    DTO_FIELD(String,   biz_status);
    DTO_FIELD(String,   thread_id);
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto
