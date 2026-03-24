#pragma once

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

class PodIdDto : public oatpp::DTO {
    DTO_INIT(PodIdDto, DTO)

    DTO_FIELD(String, pod_id);
};

class PodPtzPoseDto : public oatpp::DTO {
    DTO_INIT(PodPtzPoseDto, DTO)

    DTO_FIELD(Float64, yaw);
    DTO_FIELD(Float64, pitch);
    DTO_FIELD(Float64, roll);
    DTO_FIELD(Float64, zoom);
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto