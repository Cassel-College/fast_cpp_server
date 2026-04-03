#pragma once

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/Types.hpp"
#include <nlohmann/json.hpp>

namespace my_api::base {

using OutgoingResponse = oatpp::web::protocol::http::outgoing::Response;
using OutgoingResponsePtr = std::shared_ptr<OutgoingResponse>;
using MyAPIResponsePtr = std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>;

class BaseApiController : public oatpp::web::server::api::ApiController {
public:
    explicit BaseApiController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

protected:
    // ====== 旧有方法 ======
    OutgoingResponsePtr ok(const oatpp::String& message);

    template <typename T>
    OutgoingResponsePtr okDto(const T& dto) {
        return createDtoResponse(Status::CODE_200, dto);
    }

    OutgoingResponsePtr badRequest(const oatpp::String& message);

    // ====== 新增：企业级 JSON 响应辅助方法 ======
    // 响应统一格式：
    // {
    //   "success": bool,
    //   "code": int,
    //   "message": string,
    //   "data": object|array,       // 可选
    //   "details": object|array     // 可选（调试信息，不建议包含敏感数据）
    // }
    inline OutgoingResponsePtr jsonResponse(const oatpp::web::protocol::http::Status& status, const nlohmann::json& body) {
        auto bodyStr = oatpp::String(body.dump().c_str());
        auto res = createResponse(status, bodyStr);
        res->putHeader("Content-Type", "application/json");
        return res;
    }

    inline OutgoingResponsePtr jsonOk(const nlohmann::json& data = nlohmann::json::object(), const std::string& message = "OK") {
        nlohmann::json j;
        j["success"] = true;
        j["code"] = 200;
        j["message"] = message;
        if (!data.is_null()) j["data"] = data;
        return jsonResponse(Status::CODE_200, j);
    }

    inline OutgoingResponsePtr jsonError(int code,
                                         const std::string& message,
                                         const nlohmann::json& details = nlohmann::json()) {
        nlohmann::json j;
        j["success"] = false;
        j["code"] = code;
        j["message"] = message;
        if (!details.is_null()) j["details"] = details;
        // Map common HTTP codes to oatpp Status constants for createResponse
        oatpp::web::protocol::http::Status status = mapStatusCode(code);
        return jsonResponse(status, j);
    }

private:
    // 把常用 http code 映射到 oatpp::Status（便于 createResponse）
    inline oatpp::web::protocol::http::Status mapStatusCode(int code) const {
        using Status = oatpp::web::protocol::http::Status;
        switch(code) {
            case 200: return Status::CODE_200;
            case 201: return Status::CODE_201;
            case 202: return Status::CODE_202;
            case 204: return Status::CODE_204;
            case 400: return Status::CODE_400;
            case 401: return Status::CODE_401;
            case 403: return Status::CODE_403;
            case 404: return Status::CODE_404;
            case 409: return Status::CODE_409;
            case 413: return Status::CODE_413;
            case 422: return Status::CODE_422;
            case 500: return Status::CODE_500;
            default:  return Status::CODE_500;
        }
    }
};

} // namespace my_api::base