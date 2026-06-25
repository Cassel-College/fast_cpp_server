/**
 * @file ContextController.cpp
 * @brief 运行时上下文（MyContext）REST API 控制器实现
 */

#include "ContextController.h"

#include <nlohmann/json.hpp>

#include "MyLog.h"
#include "MyContext.h"

namespace my_api::context_api {

using namespace my_api::base;

// ============================================================================
// 构造 / 工厂
// ============================================================================

ContextController::ContextController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : BaseApiController(objectMapper) {}

std::shared_ptr<ContextController> ContextController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper) {
    return std::make_shared<ContextController>(objectMapper);
}

// ============================================================================
// 内部辅助
// ============================================================================

namespace {

/**
 * @brief 安全解析请求体为 JSON 对象
 * @return 解析成功返回 true，并写入 out；失败写入 error
 */
bool ParseBodyObject(const oatpp::String& body, nlohmann::json& out, std::string& error) {
    if (!body || body->empty()) {
        error = "请求体为空";
        return false;
    }
    nlohmann::json parsed = nlohmann::json::parse(body->c_str(), nullptr, false);
    if (parsed.is_discarded()) {
        error = "请求体不是合法的 JSON";
        return false;
    }
    if (!parsed.is_object()) {
        error = "请求体必须为 JSON 对象";
        return false;
    }
    out = std::move(parsed);
    return true;
}

}  // namespace

// ============================================================================
// 存活检查
// ============================================================================

MyAPIResponsePtr ContextController::getOnline() {
    MYLOG_INFO("[API-Context] GET /v1/context/online");
    return jsonOk({{"status", "alive"}}, "context api alive");
}

// ============================================================================
// 获取所有上下文
// ============================================================================

MyAPIResponsePtr ContextController::getAllContext() {
    MYLOG_INFO("[API-Context] GET /v1/context/all");
    try {
        nlohmann::json data = my_comm::MyContext::GetInstance().GetAllJson();
        return jsonOk(data, "获取全部上下文成功");
    } catch (const std::exception& e) {
        MYLOG_ERROR("[API-Context] 获取全部上下文失败: {}", e.what());
        return jsonError(500, std::string("获取全部上下文失败: ") + e.what());
    }
}

// ============================================================================
// 查看某个上下文
// ============================================================================

MyAPIResponsePtr ContextController::getContext(
    const oatpp::Object<my_api::dto::ContextNameRequestDto>& requestDto) {
    try {
        if (!requestDto || !requestDto->name || requestDto->name->empty()) {
            return jsonError(400, "缺少 name 字段");
        }
        const std::string name = requestDto->name->c_str();
        MYLOG_INFO("[API-Context] POST /v1/context/get name={}", name);

        nlohmann::json entry;
        if (!my_comm::MyContext::GetInstance().GetEntryJson(name, entry)) {
            return jsonError(404, "上下文不存在: " + name);
        }
        return jsonOk(entry, "查询上下文成功");
    } catch (const std::exception& e) {
        MYLOG_ERROR("[API-Context] 查询上下文失败: {}", e.what());
        return jsonError(500, std::string("查询上下文失败: ") + e.what());
    }
}

// ============================================================================
// 设置（upsert）上下文
// ============================================================================

MyAPIResponsePtr ContextController::setContext(const oatpp::String& body) {
    try {
        nlohmann::json j;
        std::string parse_err;
        if (!ParseBodyObject(body, j, parse_err)) {
            return jsonError(400, parse_err);
        }
        if (!j.contains("name") || !j["name"].is_string() || j["name"].get<std::string>().empty()) {
            return jsonError(400, "缺少有效的 name 字段");
        }
        if (!j.contains("value")) {
            return jsonError(400, "缺少 value 字段");
        }
        const std::string name = j["name"].get<std::string>();
        std::string description;
        if (j.contains("description") && j["description"].is_string()) {
            description = j["description"].get<std::string>();
        }
        MYLOG_INFO("[API-Context] POST /v1/context/set name={}", name);

        if (!my_comm::MyContext::GetInstance().Set(name, j["value"], description)) {
            return jsonError(500, "设置上下文失败");
        }
        nlohmann::json entry;
        my_comm::MyContext::GetInstance().GetEntryJson(name, entry);
        return jsonOk(entry, "设置上下文成功");
    } catch (const std::exception& e) {
        MYLOG_ERROR("[API-Context] 设置上下文失败: {}", e.what());
        return jsonError(500, std::string("设置上下文失败: ") + e.what());
    }
}

// ============================================================================
// 修改某个已存在的上下文
// ============================================================================

MyAPIResponsePtr ContextController::updateContext(const oatpp::String& body) {
    try {
        nlohmann::json j;
        std::string parse_err;
        if (!ParseBodyObject(body, j, parse_err)) {
            return jsonError(400, parse_err);
        }
        if (!j.contains("name") || !j["name"].is_string() || j["name"].get<std::string>().empty()) {
            return jsonError(400, "缺少有效的 name 字段");
        }
        if (!j.contains("value")) {
            return jsonError(400, "缺少 value 字段");
        }
        const std::string name = j["name"].get<std::string>();
        MYLOG_INFO("[API-Context] POST /v1/context/update name={}", name);

        std::string err;
        if (!my_comm::MyContext::GetInstance().UpdateValue(name, j["value"], err)) {
            return jsonError(404, err);
        }
        if (j.contains("description") && j["description"].is_string()) {
            std::string desc_err;
            my_comm::MyContext::GetInstance().UpdateDescription(
                name, j["description"].get<std::string>(), desc_err);
        }
        nlohmann::json entry;
        my_comm::MyContext::GetInstance().GetEntryJson(name, entry);
        return jsonOk(entry, "修改上下文成功");
    } catch (const std::exception& e) {
        MYLOG_ERROR("[API-Context] 修改上下文失败: {}", e.what());
        return jsonError(500, std::string("修改上下文失败: ") + e.what());
    }
}

// ============================================================================
// 删除某个上下文
// ============================================================================

MyAPIResponsePtr ContextController::deleteContext(
    const oatpp::Object<my_api::dto::ContextNameRequestDto>& requestDto) {
    try {
        if (!requestDto || !requestDto->name || requestDto->name->empty()) {
            return jsonError(400, "缺少 name 字段");
        }
        const std::string name = requestDto->name->c_str();
        MYLOG_INFO("[API-Context] POST /v1/context/delete name={}", name);

        if (!my_comm::MyContext::GetInstance().Erase(name)) {
            return jsonError(404, "上下文不存在: " + name);
        }
        return jsonOk(nlohmann::json::object(), "删除上下文成功");
    } catch (const std::exception& e) {
        MYLOG_ERROR("[API-Context] 删除上下文失败: {}", e.what());
        return jsonError(500, std::string("删除上下文失败: ") + e.what());
    }
}

}  // namespace my_api::context_api
