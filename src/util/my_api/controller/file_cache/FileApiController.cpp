/**
 * @file FileApiController.cpp
 * @brief 文件缓存管理 REST API 控制器 —— 实现文件
 *
 * 实现逻辑与网络传输层解耦：
 *   - Controller 仅负责参数校验、请求解析/序列化、HTTP 状态码映射
 *   - 文件操作全部委托给 MyCacheProvider / MyCache
 *   - 校验和映射逻辑在 FileApiHelpers.h 中，可独立测试
 */

#include "FileApiController.h"
#include "FileApiHelpers.h"
#include "MyCacheProvider.h"
#include "MyLog.h"

#include "oatpp/encoding/Base64.hpp"
#include "oatpp/web/mime/multipart/InMemoryDataProvider.hpp"
#include "oatpp/web/mime/multipart/PartList.hpp"
#include "oatpp/web/mime/multipart/Reader.hpp"
#include "oatpp/web/protocol/http/Http.hpp"

using namespace my_api::file_cache_api;
using namespace my_api::base;
using namespace my_cache;
using json = nlohmann::json;

namespace {

std::string PartToString(const std::shared_ptr<oatpp::web::mime::multipart::Part>& part) {
    if (!part || !part->getPayload()) {
        return {};
    }

    auto data = part->getPayload()->getInMemoryData();
    if (!data) {
        return {};
    }

    return std::string(data->c_str(), data->size());
}

std::vector<uint8_t> PartToBytes(const std::shared_ptr<oatpp::web::mime::multipart::Part>& part) {
    if (!part || !part->getPayload()) {
        return {};
    }

    auto data = part->getPayload()->getInMemoryData();
    if (!data) {
        return {};
    }

    const auto* begin = reinterpret_cast<const uint8_t*>(data->data());
    return std::vector<uint8_t>(begin, begin + data->size());
}

bool IsMultipartFormData(const oatpp::String& content_type) {
    return content_type && std::string(content_type->c_str()).find("multipart/form-data") != std::string::npos;
}

}  // namespace

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

MyAPIResponsePtr FileApiController::uploadFile(
    const oatpp::Object<my_api::dto::CacheUploadRequestDto>& requestDto) {
    MYLOG_INFO("[FileApiController] uploadFile 请求收到");

    if (!requestDto || !requestDto->filename || !requestDto->data) {
        return jsonError(400, "缺少必需字段 filename 或类型错误");
    }
    std::string filename = requestDto->filename->c_str();

    std::string filename_err;
    if (!ValidateFilename(filename, filename_err)) {
        MYLOG_WARN("[FileApiController] filename 校验失败：{}", filename_err);
        return jsonError(400, "filename 校验失败", {{"detail", filename_err}});
    }

    if (requestDto->data->empty()) {
        return jsonError(400, "缺少必需字段 data 或类型错误");
    }
    std::string data_b64 = requestDto->data->c_str();

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

    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    auto save_result = cache_result.value->SaveFile(filename, file_data);
    if (!save_result.Ok()) {
        int http_code = CacheErrorToHttpCode(save_result.code);
        MYLOG_WARN("[FileApiController] 文件保存失败：filename={}, 错误={}", filename, CacheErrorCodeToString(save_result.code));
        return jsonError(http_code,
                         "文件保存失败",
                         {{"error_code", CacheErrorCodeToString(save_result.code)},
                          {"filename", filename}});
    }

    MYLOG_INFO("[FileApiController] 文件上传成功：filename={}, 大小={} 字节", filename, file_data.size());
    return jsonOk({{"filename", filename}, {"size", file_data.size()}}, "文件上传成功");
}

// ============================================================================
// POST /v1/cache/upload-local
// ============================================================================

MyAPIResponsePtr FileApiController::uploadLocalFile(
    const std::shared_ptr<IncomingRequest>& request) {
    MYLOG_INFO("[FileApiController] uploadLocalFile 请求收到");

    if (!request) {
        return jsonError(400, "请求对象无效");
    }

    const auto content_type = request->getHeader(oatpp::web::protocol::http::Header::CONTENT_TYPE);
    if (!IsMultipartFormData(content_type)) {
        return jsonError(400, "Content-Type 必须为 multipart/form-data");
    }

    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    auto multipart = std::make_shared<oatpp::web::mime::multipart::PartList>(request->getHeaders());
    oatpp::web::mime::multipart::Reader multipart_reader(multipart.get());
    multipart_reader.setDefaultPartReader(
        oatpp::web::mime::multipart::createInMemoryPartReader(-1));

    try {
        request->transferBody(&multipart_reader);
    } catch (oatpp::web::protocol::http::HttpError& e) {
        MYLOG_WARN("[FileApiController] multipart 解析失败：status={}, msg={}",
                   e.getInfo().status.code,
                   e.what());
        return jsonError(e.getInfo().status.code,
                         "multipart 请求解析失败",
                         {{"detail", e.what()}});
    } catch (const std::exception& e) {
        MYLOG_WARN("[FileApiController] multipart 解析异常：{}", e.what());
        return jsonError(400, "multipart 请求解析失败", {{"detail", e.what()}});
    }

    auto file_part = multipart->getNamedPart("file");
    if (!file_part) {
        return jsonError(400, "缺少必需文件字段 file");
    }

    std::string file_name = PartToString(multipart->getNamedPart("file_name"));
    if (file_name.empty() && file_part->getFilename()) {
        file_name.assign(file_part->getFilename()->c_str(), file_part->getFilename()->size());
    }

    std::string filename_err;
    if (!ValidateFilename(file_name, filename_err)) {
        MYLOG_WARN("[FileApiController] file_name 校验失败：{}", filename_err);
        return jsonError(400, "file_name 校验失败", {{"detail", filename_err}});
    }

    if (!file_part->getPayload()) {
        return jsonError(400, "上传文件内容为空");
    }

    auto file_data = PartToBytes(file_part);
    auto save_result = cache_result.value->SaveFile(file_name, file_data);
    if (!save_result.Ok()) {
        const int http_code = CacheErrorToHttpCode(save_result.code);
        MYLOG_WARN("[FileApiController] 客户端文件导入失败：file_name={}, 错误={}",
                   file_name, CacheErrorCodeToString(save_result.code));
        return jsonError(http_code,
                         "文件导入失败",
                         {{"error_code", CacheErrorCodeToString(save_result.code)},
                          {"file_name", file_name}});
    }

    MYLOG_INFO("[FileApiController] 客户端本地文件导入成功：file_name={}, 大小={} 字节",
               file_name, file_data.size());
    return jsonOk({{"file_name", file_name},
                   {"size", file_data.size()}},
                  "文件导入成功");
}

// ============================================================================
// POST /v1/cache/query
// ============================================================================

MyAPIResponsePtr FileApiController::queryFile(
    const oatpp::Object<my_api::dto::CacheFileRequestDto>& requestDto) {
    MYLOG_INFO("[FileApiController] queryFile 请求收到");

    if (!requestDto || !requestDto->filename) {
        return jsonError(400, "缺少必需字段 filename 或类型错误");
    }
    std::string filename = requestDto->filename->c_str();

    std::string filename_err;
    if (!ValidateFilename(filename, filename_err)) {
        MYLOG_WARN("[FileApiController] filename 校验失败：{}", filename_err);
        return jsonError(400, "filename 校验失败", {{"detail", filename_err}});
    }

    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    auto exists_result = cache_result.value->Exists(filename);
    if (!exists_result.Ok()) {
        int http_code = CacheErrorToHttpCode(exists_result.code);
        return jsonError(http_code,
                         "文件查询失败",
                         {{"error_code", CacheErrorCodeToString(exists_result.code)}});
    }

    std::string full_path;
    if (exists_result.value) {
        auto path_result = cache_result.value->GetFullPath(filename);
        if (path_result.Ok()) {
            full_path = path_result.value;
        }
    }

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

MyAPIResponsePtr FileApiController::deleteFile(
    const oatpp::Object<my_api::dto::CacheFileRequestDto>& requestDto) {
    MYLOG_INFO("[FileApiController] deleteFile 请求收到");

    if (!requestDto || !requestDto->filename) {
        return jsonError(400, "缺少必需字段 filename 或类型错误");
    }
    std::string filename = requestDto->filename->c_str();

    std::string filename_err;
    if (!ValidateFilename(filename, filename_err)) {
        MYLOG_WARN("[FileApiController] filename 校验失败：{}", filename_err);
        return jsonError(400, "filename 校验失败", {{"detail", filename_err}});
    }

    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    auto del_result = cache_result.value->DeleteFile(filename);
    if (!del_result.Ok()) {
        int http_code = CacheErrorToHttpCode(del_result.code);
        MYLOG_WARN("[FileApiController] 文件删除失败：filename={}, 错误={}", filename, CacheErrorCodeToString(del_result.code));
        return jsonError(http_code,
                         "文件删除失败",
                         {{"error_code", CacheErrorCodeToString(del_result.code)},
                          {"filename", filename}});
    }

    MYLOG_INFO("[FileApiController] 文件删除成功：filename={}", filename);
    return jsonOk({{"filename", filename}}, "文件删除成功");
}

// ============================================================================
// POST /v1/cache/list
// ============================================================================

MyAPIResponsePtr FileApiController::listFiles(const oatpp::Object<my_api::dto::CacheListRequestDto>& requestDto) {
    MYLOG_INFO("[FileApiController] listFiles 请求收到");

    const std::string folder_path = requestDto && requestDto->folder_path
        ? requestDto->folder_path->c_str()
        : std::string();

    // 1. 获取 MyCache 实例
    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    // 2. 获取指定目录下的文件列表
    auto list_result = cache_result.value->GetFileList(folder_path);
    if (!list_result.Ok()) {
        int http_code = CacheErrorToHttpCode(list_result.code);
        MYLOG_WARN("[FileApiController] 获取文件列表失败：folder_path={}, 错误={}",
                   folder_path, CacheErrorCodeToString(list_result.code));
        return jsonError(http_code,
                         "获取文件列表失败",
                         {{"error_code", CacheErrorCodeToString(list_result.code)},
                          {"folder_path", folder_path}});
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

    MYLOG_INFO("[FileApiController] 文件列表查询成功：folder_path={}, 共 {} 个文件",
               folder_path, list_result.value.size());
    return jsonOk({{"folder_path", folder_path.empty() ? cache_result.value->GetRootPath() : folder_path},
                   {"files", files_array},
                   {"total", list_result.value.size()}},
                  "查询成功");
}

// ============================================================================
// POST /v1/cache/create-folder
// ============================================================================

MyAPIResponsePtr FileApiController::createFolder(
    const oatpp::Object<my_api::dto::CacheCreateFolderRequestDto>& requestDto) {
    MYLOG_INFO("[FileApiController] createFolder 请求收到");

    if (!requestDto || !requestDto->new_folder_name || requestDto->new_folder_name->empty()) {
        return jsonError(400, "缺少必需字段 new_folder_name 或类型错误");
    }

    const std::string folder_path = requestDto->folder_path
        ? requestDto->folder_path->c_str()
        : std::string();
    const std::string new_folder_name = requestDto->new_folder_name->c_str();

    auto cache_result = MyCacheProvider::Get();
    if (!cache_result.Ok()) {
        MYLOG_ERROR("[FileApiController] MyCache 未初始化");
        return jsonError(CacheErrorToHttpCode(cache_result.code),
                         "文件缓存服务未初始化",
                         {{"error_code", CacheErrorCodeToString(cache_result.code)}});
    }

    auto create_result = cache_result.value->CreateSubdirectory(folder_path, new_folder_name);
    if (!create_result.Ok()) {
        int http_code = CacheErrorToHttpCode(create_result.code);
        MYLOG_WARN("[FileApiController] 创建子目录失败：folder_path={}, new_folder_name={}, 错误={}",
                   folder_path, new_folder_name, CacheErrorCodeToString(create_result.code));
        return jsonError(http_code,
                         "创建子目录失败",
                         {{"error_code", CacheErrorCodeToString(create_result.code)},
                          {"folder_path", folder_path},
                          {"new_folder_name", new_folder_name}});
    }

    MYLOG_INFO("[FileApiController] 创建子目录成功：{}", create_result.value);
    return jsonOk({{"folder_path", folder_path.empty() ? cache_result.value->GetRootPath() : folder_path},
                   {"new_folder_name", new_folder_name},
                   {"created_path", create_result.value}},
                  "创建子目录成功");
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
