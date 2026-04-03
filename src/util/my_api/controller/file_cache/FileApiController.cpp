/**
 * @file FileApiController.cpp
 * @brief 文件缓存管理 REST API 控制器 —— 实现文件
 *
 * 实现逻辑与网络传输层解耦：
 *   - Controller 仅负责参数校验、JSON 解析/序列化、HTTP 状态码映射
 *   - 文件操作全部委托给 MyCacheProvider / MyCache
 *   - 校验和映射逻辑在 FileApiHelpers.h 中，可独立测试
 */

#include "FileApiController.h"
#include "FileApiHelpers.h"
#include "MyCacheProvider.h"
#include "MyLog.h"
#include "validators/RequestValidators.hpp"

#include "oatpp/encoding/Base64.hpp"

using namespace my_api::file_cache_api;
using namespace my_api::base;
using namespace my_cache;
using json = nlohmann::json;

// ============================================================================
// 构造 / 工厂
// ============================================================================

FileApiController::FileApiController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : base::BaseApiController(objectMapper)
{
    MYLOG_INFO("[FileApiController] 控制器已构造");
}

std::shared_ptr<FileApiController> FileApiController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper)
{
    return std::make_shared<FileApiController>(objectMapper);
}

// ============================================================================
// POST /v1/cache/upload
// ============================================================================

MyAPIResponsePtr FileApiController::uploadFile(const oatpp::String& body) {
    MYLOG_INFO("[FileApiController] uploadFile 请求收到");

    // 1. 解析 JSON
    json req;
    std::string parse_err;
    if (!my_api::validators::parseJsonString(body->c_str(), req, parse_err)) {
        MYLOG_WARN("[FileApiController] JSON 解析失败：{}", parse_err);
        return jsonError(400, "请求体 JSON 解析失败", {{"detail", parse_err}});
    }

    // 2. 白名单 key 校验
    std::string wl_err;
    if (!my_api::validators::whitelistKeysCheck(req, {"filename", "data"}, wl_err)) {
        MYLOG_WARN("[FileApiController] 请求包含未知字段：{}", wl_err);
        return jsonError(400, "请求包含未知字段", {{"detail", wl_err}});
    }

    // 3. 提取并校验 filename
    if (!req.contains("filename") || !req["filename"].is_string()) {
        return jsonError(400, "缺少必需字段 filename 或类型错误");
    }
    std::string filename = req["filename"].get<std::string>();

    std::string filename_err;
    if (!ValidateFilename(filename, filename_err)) {
        MYLOG_WARN("[FileApiController] filename 校验失败：{}", filename_err);
        return jsonError(400, "filename 校验失败", {{"detail", filename_err}});
    }

    // 4. 提取并解码 data（Base64）
    if (!req.contains("data") || !req["data"].is_string()) {
        return jsonError(400, "缺少必需字段 data 或类型错误");
    }
    std::string data_b64 = req["data"].get<std::string>();

    if (data_b64.empty()) {
        return jsonError(400, "data 字段不能为空");
    }

    // 使用 oatpp 的 Base64 解码
    oatpp::String decoded;
    try {
        decoded = oatpp::encoding::Base64::decode(data_b64.c_str(), data_b64.size());
    } catch (...) {
        MYLOG_WARN("[FileApiController] Base64 解码失败，filename={}", filename);
        return jsonError(400, "data 字段 Base64 解码失败");
    }

    if (!decoded || decoded->empty()) {
        return jsonError(400, "data 字段 Base64 解码后为空");
    }

    // 转换为 vector<uint8_t>
    std::vector<uint8_t> file_data(decoded->begin(), decoded->end());

    // 5. 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    // 6. 调用 SaveFile
    auto save_result = cache_result.value->SaveFile(filename, file_data);
    if (!save_result.Ok()) {
        int http_code = CacheErrorToHttpCode(save_result.code);
        MYLOG_WARN("[FileApiController] 文件保存失败：filename={}, 错误={}", filename, CacheErrorCodeToString(save_result.code));
        return jsonError(http_code,
                         "文件保存失败",
                         {{"error_code", CacheErrorCodeToString(save_result.code)},
                          {"filename", filename}});
    }

    // 7. 成功响应
    MYLOG_INFO("[FileApiController] 文件上传成功：filename={}, 大小={} 字节", filename, file_data.size());
    return jsonOk({{"filename", filename}, {"size", file_data.size()}}, "文件上传成功");
}

// ============================================================================
// POST /v1/cache/query
// ============================================================================

MyAPIResponsePtr FileApiController::queryFile(const oatpp::String& body) {
    MYLOG_INFO("[FileApiController] queryFile 请求收到");

    // 1. 解析 JSON
    json req;
    std::string parse_err;
    if (!my_api::validators::parseJsonString(body->c_str(), req, parse_err)) {
        MYLOG_WARN("[FileApiController] JSON 解析失败：{}", parse_err);
        return jsonError(400, "请求体 JSON 解析失败", {{"detail", parse_err}});
    }

    // 2. 白名单 key 校验
    std::string wl_err;
    if (!my_api::validators::whitelistKeysCheck(req, {"filename"}, wl_err)) {
        MYLOG_WARN("[FileApiController] 请求包含未知字段：{}", wl_err);
        return jsonError(400, "请求包含未知字段", {{"detail", wl_err}});
    }

    // 3. 提取并校验 filename
    if (!req.contains("filename") || !req["filename"].is_string()) {
        return jsonError(400, "缺少必需字段 filename 或类型错误");
    }
    std::string filename = req["filename"].get<std::string>();

    std::string filename_err;
    if (!ValidateFilename(filename, filename_err)) {
        MYLOG_WARN("[FileApiController] filename 校验失败：{}", filename_err);
        return jsonError(400, "filename 校验失败", {{"detail", filename_err}});
    }

    // 4. 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    // 5. 查询文件是否存在
    auto exists_result = cache_result.value->Exists(filename);
    if (!exists_result.Ok()) {
        int http_code = CacheErrorToHttpCode(exists_result.code);
        return jsonError(http_code,
                         "文件查询失败",
                         {{"error_code", CacheErrorCodeToString(exists_result.code)}});
    }

    // 6. 获取完整路径
    std::string full_path;
    if (exists_result.value) {
        auto path_result = cache_result.value->GetFullPath(filename);
        if (path_result.Ok()) {
            full_path = path_result.value;
        }
    }

    // 7. 响应
    json resp_data;
    resp_data["exists"] = exists_result.value;
    resp_data["filename"] = filename;
    if (!full_path.empty()) {
        resp_data["full_path"] = full_path;
    }

    MYLOG_INFO("[FileApiController] 文件查询完成：filename={}, exists={}", filename, exists_result.value);
    return jsonOk(resp_data, "查询成功");
}

// ============================================================================
// POST /v1/cache/delete
// ============================================================================

MyAPIResponsePtr FileApiController::deleteFile(const oatpp::String& body) {
    MYLOG_INFO("[FileApiController] deleteFile 请求收到");

    // 1. 解析 JSON
    json req;
    std::string parse_err;
    if (!my_api::validators::parseJsonString(body->c_str(), req, parse_err)) {
        MYLOG_WARN("[FileApiController] JSON 解析失败：{}", parse_err);
        return jsonError(400, "请求体 JSON 解析失败", {{"detail", parse_err}});
    }

    // 2. 白名单 key 校验
    std::string wl_err;
    if (!my_api::validators::whitelistKeysCheck(req, {"filename"}, wl_err)) {
        MYLOG_WARN("[FileApiController] 请求包含未知字段：{}", wl_err);
        return jsonError(400, "请求包含未知字段", {{"detail", wl_err}});
    }

    // 3. 提取并校验 filename
    if (!req.contains("filename") || !req["filename"].is_string()) {
        return jsonError(400, "缺少必需字段 filename 或类型错误");
    }
    std::string filename = req["filename"].get<std::string>();

    std::string filename_err;
    if (!ValidateFilename(filename, filename_err)) {
        MYLOG_WARN("[FileApiController] filename 校验失败：{}", filename_err);
        return jsonError(400, "filename 校验失败", {{"detail", filename_err}});
    }

    // 4. 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    // 5. 调用 DeleteFile
    auto del_result = cache_result.value->DeleteFile(filename);
    if (!del_result.Ok()) {
        int http_code = CacheErrorToHttpCode(del_result.code);
        MYLOG_WARN("[FileApiController] 文件删除失败：filename={}, 错误={}", filename, CacheErrorCodeToString(del_result.code));
        return jsonError(http_code,
                         "文件删除失败",
                         {{"error_code", CacheErrorCodeToString(del_result.code)},
                          {"filename", filename}});
    }

    // 6. 成功响应
    MYLOG_INFO("[FileApiController] 文件删除成功：filename={}", filename);
    return jsonOk({{"filename", filename}}, "文件删除成功");
}

// ============================================================================
// POST /v1/cache/list
// ============================================================================

MyAPIResponsePtr FileApiController::listFiles(const oatpp::String& body) {
    MYLOG_INFO("[FileApiController] listFiles 请求收到");

    // 1. 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    // 2. 获取所有文件列表
    auto list_result = cache_result.value->GetAllFileList();
    if (!list_result.Ok()) {
        int http_code = CacheErrorToHttpCode(list_result.code);
        MYLOG_WARN("[FileApiController] 获取文件列表失败：错误={}", CacheErrorCodeToString(list_result.code));
        return jsonError(http_code,
                         "获取文件列表失败",
                         {{"error_code", CacheErrorCodeToString(list_result.code)}});
    }

    // 3. 构造响应
    json files_array = json::array();
    for (const auto& fi : list_result.value) {
        files_array.push_back({
            {"name", fi.name},
            {"size", fi.size},
            {"type", fi.type},
            {"modified_at", fi.modified_at}
        });
    }

    MYLOG_INFO("[FileApiController] 文件列表查询成功，共 {} 个文件", list_result.value.size());
    return jsonOk({{"files", files_array}, {"total", list_result.value.size()}}, "查询成功");
}

// ============================================================================
// GET /v1/cache/info
// ============================================================================

MyAPIResponsePtr FileApiController::cacheInfo() {
    MYLOG_INFO("[FileApiController] cacheInfo 请求收到");

    // 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    const auto& config = cache_result.value->GetConfig();
    json data;
    data["root_path"] = config.root_path;
    data["max_file_size"] = config.max_file_size;
    data["max_retention_seconds"] = config.max_retention_seconds;

    MYLOG_INFO("[FileApiController] 缓存配置查询成功");
    return jsonOk(data, "查询成功");
}

// ============================================================================
// GET /v1/cache/status
// ============================================================================

MyAPIResponsePtr FileApiController::cacheStatus() {
    MYLOG_INFO("[FileApiController] cacheStatus 请求收到");

    // 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        // 即使未初始化也返回状态信息而非错误
        json data;
        data["status"] = CacheStatusToString(CacheStatus::NotInitialized);
        data["status_code"] = static_cast<int>(CacheStatus::NotInitialized);
        return jsonOk(data, "查询成功");
    }

    auto status = cache_result.value->Status();
    json data;
    data["status"] = CacheStatusToString(status);
    data["status_code"] = static_cast<int>(status);

    MYLOG_INFO("[FileApiController] 缓存状态查询成功：{}", CacheStatusToString(status));
    return jsonOk(data, "查询成功");
}
