#include "BaseApiController.hpp"

namespace my_api::base {

BaseApiController::BaseApiController(
    const std::shared_ptr<ObjectMapper>& objectMapper
)
    : oatpp::web::server::api::ApiController(objectMapper) {}

OutgoingResponsePtr
BaseApiController::ok(const oatpp::String& message) {
    return createResponse(Status::CODE_200, message);
}

OutgoingResponsePtr
BaseApiController::badRequest(const oatpp::String& message) {
    return createResponse(Status::CODE_400, message);
}

} // namespace my_api::base
