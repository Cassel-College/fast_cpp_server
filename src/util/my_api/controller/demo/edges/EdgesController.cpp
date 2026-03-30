#include "BaseApiController.hpp"
#include "MyLog.h"
#include "MyEdgeManager.h"
#include "EdgesController.hpp"
#include "dto/demo/EdgeStatusDto.hpp"
#include "dto/demo/TaskDto.hpp"

#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <memory>


namespace my_api::edge {

using my_api::base::MyAPIResponsePtr;

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

MyAPIResponsePtr EdgesController::getEdgesStatus() {
    MYLOG_INFO("[API] 收到请求: GET /v1/edges/status");

    auto status_map = ::my_edge::MyEdgeManager::GetInstance().GetHeartbeatInfo();

    auto result =
        oatpp::Vector<oatpp::Object<my_api::dto::EdgeStatusDto>>::createShared();

    for (const auto& item : status_map.items()) {
        auto dto = my_api::dto::EdgeStatusDto::createShared();
        dto->name = item.key().c_str();
        dto->ip = item.value().value("ip", "").c_str();
        dto->online = item.value().value("online", false);
        dto->biz_status = item.value().value("biz_status", "").c_str();
        dto->thread_id = item.value().value("thread_id", "").c_str();
        result->push_back(dto);
    }

    return createDtoResponse(Status::CODE_200, result);
}

MyAPIResponsePtr EdgesController::startAllEdges() {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/startAllEdges");

    ::my_edge::MyEdgeManager::GetInstance().startAllEdges();

    return ok("All edge devices started successfully.");
}

MyAPIResponsePtr EdgesController::appendTaskToEdgeById(const oatpp::Object<my_api::dto::TaskDto>& taskDto) {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/appendTaskToEdgeById (DTO -> unified JSON response)");

    try {
        // 1) DTO 基本校验
        if (!taskDto) {
            MYLOG_WARN("请求体 DTO 解析失败或为空");
            return jsonError(400, "invalid request body");
        } else {
            MYLOG_INFO("请求体 DTO 解析成功");
        }

        // 2) 必填字段 edgeId
        std::string edgeId = taskDto->edgeId ? taskDto->edgeId->c_str() : "";
        if (edgeId.empty()) {
            MYLOG_WARN("edgeId 缺失");
            return jsonError(400, "edgeId required");
        } else {
            MYLOG_INFO("必填字段 edgeId: {}", edgeId);
        }

        // 3) 可选字段
        std::string taskType = taskDto->taskType ? taskDto->taskType->c_str() : "default";
        std::string paramsJsonStr = taskDto->paramsJson ? taskDto->paramsJson->c_str() : "{}";

        // 4) 简单长度限制（可根据企业标准调整）
        if (edgeId.size() > 256) {
            return jsonError(400, "edgeId too long");
        }
        if (taskType.size() > 128) {
            return jsonError(400, "taskType too long");
        }
        if (paramsJsonStr.size() > 64 * 1024) {
            return jsonError(400, "paramsJson too large");
        }

        // 5) 解析 paramsJson（若解析失败，返回 400 并把解析错误放到 details）
        nlohmann::json params;
        try {
            params = nlohmann::json::parse(paramsJsonStr);
            MYLOG_INFO("paramsJson 解析成功: {}", params.dump());
        } catch (const nlohmann::json::parse_error& e) {
            MYLOG_WARN("[EdgesController.cpp:appendTaskToEdgeById] paramsJson 解析失败: {}", e.what());
            nlohmann::json details = {
                {"parse_error", e.what()},
                {"input_preview", paramsJsonStr.size() > 256 ? paramsJsonStr.substr(0,256) : paramsJsonStr}
            };
            return jsonError(400, "paramsJson must be valid JSON", details);
        }
        // 构造 my_data::Task 对象
        my_data::Task task;
        taskDto->taskData->toTask(task);
        MYLOG_INFO("构造 my_data::Task 对象成功: {}", task.toString());
        
        // 6) 将任务传给业务层（示例调用）；业务层返回 bool 或可扩展为错误信息结构
        // bool ok = my_edge::MyEdgeManager::GetInstance().appendTaskToEdgeById(edgeId, params);
        bool ok = my_edge::MyEdgeManager::GetInstance().appendTaskToEdgeByIdV2(edgeId, task);

        if (!ok) {
            MYLOG_ERROR("AppendTaskToEdgeById 业务执行失败, edgeId={}", edgeId);
            return jsonError(500, "append task failed");
        } else {
            MYLOG_INFO("AppendTaskToEdgeById 业务执行成功, edgeId={}", edgeId);
        }

        // 7) 成功返回统一 JSON（包含 data）
        nlohmann::json data = {
            {"edgeId", edgeId},
            {"taskType", taskType}
        };
        return jsonOk(data, "append task success");

    } catch (const std::exception& e) {
        // 捕获并记录异常，同时返回 500 统一 JSON
        MYLOG_ERROR("[EdgesController::appendTaskToEdgeById] exception: {}", e.what());
        nlohmann::json details = { {"exception", e.what()} };
        return jsonError(500, "internal server error", details);
    }
}

MyAPIResponsePtr EdgesController::getOnlineEdges() {
    MYLOG_INFO("[API] 收到请求: GET /v1/edges/getOnlineEdges");

    std::vector<std::string> online_edges;
    my_edge::MyEdgeManager::GetInstance().getOnlineEdges(online_edges);

    auto result = oatpp::Vector<oatpp::String>::createShared();
    for (const auto& edge_id : online_edges) {
        result->push_back(edge_id.c_str());
    }

    return createDtoResponse(Status::CODE_200, result);
}

MyAPIResponsePtr EdgesController::getTargetEdgeDumpInternalInfo(
    const oatpp::Object<my_api::dto::EdgeIDDto>& edgeIdDto
) {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/dumpInternalInfo");

    if (!edgeIdDto || !edgeIdDto->edgeId) {
        MYLOG_WARN("请求体 DTO 解析失败或 edgeId 缺失");
        return jsonError(400, "edgeId required");
    }

    std::string edgeId = edgeIdDto->edgeId->c_str();
    if (edgeId.size() > 256) {
        return jsonError(400, "edgeId too long");
    }

    nlohmann::json dump_info;
    bool found = my_edge::MyEdgeManager::GetInstance().getEdgeInternalDumpInfo(edgeId, dump_info);
    if (!found) {
        MYLOG_WARN("Edge ID '{}' 未找到，无法获取内部信息 Dump", edgeId);
        return jsonError(404, "edge not found");
    }

    return jsonOk(dump_info, "dump info retrieved");

}; // class EdgesController


MyAPIResponsePtr EdgesController::getTargetEdgeRunTimeStatusInfo (
    const oatpp::Object<my_api::dto::EdgeIDDto>& edgeIdDto
) {
    MYLOG_INFO("[API] 收到请求: POST /v1/edges/getTargetEdgeRunTimeStatusInfo");

    if (!edgeIdDto || !edgeIdDto->edgeId) {
        MYLOG_WARN("请求体 DTO 解析失败或 edgeId 缺失");
        return jsonError(400, "edgeId required");
    }

    std::string edgeId = edgeIdDto->edgeId->c_str();
    if (edgeId.size() > 256) {
        return jsonError(400, "edgeId too long");
    }

    nlohmann::json status_info;
    bool found = my_edge::MyEdgeManager::GetInstance().getEdgeRunTimeStatusInfo(edgeId, status_info);
    if (!found) {
        MYLOG_WARN("Edge ID '{}' 未找到，无法获取运行时状态信息", edgeId);
        return jsonError(404, "edge not found");
    }

    return jsonOk(status_info, "run time status info retrieved");
}
}; // namespace my_api::edge