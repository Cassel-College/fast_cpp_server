#pragma once

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/Types.hpp"

namespace my_api::base {

using OutgoingResponse = oatpp::web::protocol::http::outgoing::Response;
using OutgoingResponsePtr = std::shared_ptr<OutgoingResponse>;


class BaseApiController : public oatpp::web::server::api::ApiController {
public:
    explicit BaseApiController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

protected:
    // ====== 正确的返回类型 ======
    OutgoingResponsePtr ok(const oatpp::String& message);

    template <typename T>
    OutgoingResponsePtr okDto(const T& dto) {
        return createDtoResponse(Status::CODE_200, dto);
    }

    OutgoingResponsePtr badRequest(const oatpp::String& message);
};

} // namespace my_api::base
