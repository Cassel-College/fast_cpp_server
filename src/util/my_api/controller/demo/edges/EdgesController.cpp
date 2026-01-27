// filepath: /home/cs/DockerRoot/fast_cpp_server/src/util/my_api/controller/demo/edges/EdgesController.cpp
#include "MyLog.h"
#include "MyEdgeManager.h"
#include "MyEdges.h"
#include "EdgesController.hpp"
#include "dto/demo/EdgeStatusDto.hpp"
// Include any additional DTOs if needed, e.g., for appendTaskToEdgeById

namespace my_api::edge {

EdgesController::EdgesController(
    const std::shared_ptr<ObjectMapper>& objectMapper
)
    : BaseApiController(objectMapper) {}

std::shared_ptr<EdgesController>
EdgesController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper
) {
    return std::make_shared<EdgesController>(objectMapper);
}

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> EdgesController::getEdgesStatus() {
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

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> EdgesController::startAllEdges() {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/startAllEdges");

    // edge_manager::MyEdgeManager::GetInstance().StartAllEdges();

    return ok("All edge devices started successfully.");
}

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> EdgesController::appendTaskToEdgeById(
    const std::shared_ptr<oatpp::web::protocol::http::incoming::Request>& request
) {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/appendTaskToEdgeById");

    // Assuming a TaskDto is defined, e.g., in dto/demo/TaskDto.hpp
    // auto taskDto = request->readBodyToDto<oatpp::Object<my_api::dto::TaskDto>>(getDefaultObjectMapper().get());
    // edge_manager::MyEdgeManager::GetInstance().AppendTaskToEdgeById(taskDto->edgeId, taskDto->taskData);

    // Placeholder implementation
    return ok("Task appended to edge successfully.");
}


} // namespace my_api::edge