#pragma once

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

class PodIdDto : public oatpp::DTO {
    DTO_INIT(PodIdDto, DTO)

    DTO_FIELD(String, pod_id);
};

class PodCapabilityQueryDto : public oatpp::DTO {
    DTO_INIT(PodCapabilityQueryDto, DTO)

    DTO_FIELD(String, pod_id);
    DTO_FIELD(Any, capability_type);
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto