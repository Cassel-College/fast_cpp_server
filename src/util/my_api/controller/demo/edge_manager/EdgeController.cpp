#include "MyLog.h"
#include "MyEdgeManager.h"
#include "EdgeController.hpp"
#include "dto/demo/EdgeStatusDto.hpp"

namespace my_api::edge {

EdgeController::EdgeController(
    const std::shared_ptr<ObjectMapper>& objectMapper
)
    : BaseApiController(objectMapper) {}

std::shared_ptr<EdgeController>
EdgeController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper
) {
    return std::make_shared<EdgeController>(objectMapper);
}

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> EdgeController::getEdgesStatus() {
    MYLOG_INFO("[API] 收到请求: GET /v1/edges/status");

    auto status_list =
        edge_manager::MyEdgeManager::GetInstance().ShowEdgesStatus();

    auto result =
        oatpp::Vector<oatpp::Object<my_api::dto::EdgeStatusDto>>::createShared();

    for (const auto& item : status_list) {
        auto dto = my_api::dto::EdgeStatusDto::createShared();
        dto->name = item.value("name", "").c_str();
        dto->ip = item.value("ip", "").c_str();
        dto->online = item.value("online", false);
        dto->biz_status = item.value("biz_status", "").c_str();
        dto->thread_id = item.value("thread_id", "").c_str();
        result->push_back(dto);
    }

    return createDtoResponse(Status::CODE_200, result);
}

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> EdgeController::startDevice(const oatpp::String& name) {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/{}/start", name->c_str());

    edge_manager::MyEdgeManager::GetInstance().StartDevice(name->c_str());

    return ok("Edge device started successfully.");
   
}

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> EdgeController::stopDevice(const oatpp::String& name) {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/{}/stop", name->c_str());

    edge_manager::MyEdgeManager::GetInstance().StopDevice(name->c_str());

    return ok("Edge device stopped successfully.");
}



} // namespace my_api::edge
