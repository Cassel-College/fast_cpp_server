#pragma once

#include "BaseApiController.hpp"
#include "dto/demo/EdgeStatusDto.hpp"
#include "dto/demo/TaskDto.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"



namespace my_api::tuna {

#include OATPP_CODEGEN_BEGIN(ApiController)

class TunaController : public base::BaseApiController {
public:
    static constexpr const char* SWAGGER_TAG = "TunaController";
    explicit TunaController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<TunaController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper);

    ENDPOINT_INFO(tunaDown) {
            info->addTag(SWAGGER_TAG);
      info->summary = "让金枪鱼潜航器下潜";
      info->description = "让金枪鱼潜航器下潜到水下指定深度，";
      info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
      info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
      info->addResponse<oatpp::Vector<oatpp::String>>(Status::CODE_200, "application/json");
    }
    ENDPOINT("GET",
             "/v1/tuna/down",
             tunaDown
            );

    ENDPOINT_INFO(tunaUp) {
        info->addTag(SWAGGER_TAG);
        info->summary = "让金枪鱼潜航器上浮";
        info->description = "让金枪鱼潜航器上浮到水面，";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
    }
    ENDPOINT("POST",
             "/v1/tuna/up",
             tunaUp
            );

}; // class TunaController
#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::edge