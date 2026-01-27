#pragma once

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::edge {

#include OATPP_CODEGEN_BEGIN(ApiController)

class EdgeController : public base::BaseApiController {
public:
    explicit EdgeController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<EdgeController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT("GET", "/v1/edges/status", getEdgesStatus);

    ENDPOINT("POST", "/v1/edges/{name}/start", startDevice, PATH(String, name));

    ENDPOINT("POST", "/v1/edges/{name}/stop", stopDevice, PATH(String, name));
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::edge
