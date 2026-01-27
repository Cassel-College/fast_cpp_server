#pragma once

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::edge {

#include OATPP_CODEGEN_BEGIN(ApiController)

class EdgesController : public base::BaseApiController {
public:
    explicit EdgesController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<EdgesController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT("GET", "/v1/edges/status", getEdgesStatus);

    ENDPOINT("POST", "/v1/edges/startAllEdges", startAllEdges);

    ENDPOINT("POST", 
             "/v1/edges/appendTaskToEdgeById", 
             appendTaskToEdgeById, 
             REQUEST(std::shared_ptr<oatpp::web::protocol::http::incoming::Request>, request)
            );
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::edge
