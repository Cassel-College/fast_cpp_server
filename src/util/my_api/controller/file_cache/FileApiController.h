#pragma once

/**
 * @file FileApiController.h
 * @brief 文件缓存管理 REST API 控制器
 *
 * 提供以下接口：
 *   - POST /v1/cache/upload  —— 上传文件到缓存目录
 *   - POST /v1/cache/upload-local —— 将客户端本地文件导入缓存目录
 *   - POST /v1/cache/query   —— 查询文件是否存在及完整路径
 *   - POST /v1/cache/delete  —— 删除缓存中的文件
 *   - POST /v1/cache/list    —— 获取所有文件列表
 *   - GET  /v1/cache/info    —— 获取缓存配置信息
  *   - POST /v1/cache/create-folder —— 创建子目录
 *   - GET  /v1/cache/status  —— 获取缓存运行状态
 *
 * 除 upload-local 使用 multipart/form-data 外，其余写接口使用 JSON Body，禁止路径变量。
 * 底层调用 MyCacheProvider::Get() 获取 MyCache 实例。
 */

#include "BaseApiController.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "dto/file_cache/FileCacheDto.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::file_cache_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class FileApiController : public base::BaseApiController {
public:
    explicit FileApiController(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    static std::shared_ptr<FileApiController> createShared(
        const std::shared_ptr<ObjectMapper>& objectMapper
    );

    // ====================================================================
    // POST /v1/cache/upload —— 上传文件
    // ====================================================================
    ENDPOINT_INFO(uploadFile) {
        info->summary = "上传文件到缓存目录";
        info->description =
            "接收 JSON Body，将文件数据保存到缓存目录。\n"
            "data 字段为 Base64 编码的文件二进制内容。\n"
            "支持子目录路径，如 \"images/photo.jpg\"。";
        info->addConsumes<oatpp::Object<my_api::dto::CacheUploadRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST", "/v1/cache/upload", uploadFile,
             BODY_DTO(oatpp::Object<my_api::dto::CacheUploadRequestDto>, requestDto));

    // ====================================================================
    // POST /v1/cache/upload-local —— 接收客户端本地文件上传
    // ====================================================================
    ENDPOINT_INFO(uploadLocalFile) {
        info->summary = "将客户端本地文件导入缓存目录";
        info->description =
            "接收 multipart/form-data 请求。\n"
            "必须包含 file 文件字段，可选包含 file_name 文本字段作为缓存目标名称。\n"
            "若 file_name 为空，则默认使用上传文件原始文件名。";
        info->addConsumes<oatpp::Object<my_api::dto::CacheUploadLocalMultipartDto>>("multipart/form-data");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_413, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST", "/v1/cache/upload-local", uploadLocalFile,
             REQUEST(std::shared_ptr<IncomingRequest>, request));

    // ====================================================================
    // POST /v1/cache/query —— 查询文件
    // ====================================================================
    ENDPOINT_INFO(queryFile) {
        info->summary = "查询缓存文件是否存在";
        info->description =
            "接收 JSON Body，查询文件是否存在于缓存中，\n"
            "返回存在性和完整绝对路径。";
        info->addConsumes<oatpp::Object<my_api::dto::CacheFileRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST", "/v1/cache/query", queryFile,
             BODY_DTO(oatpp::Object<my_api::dto::CacheFileRequestDto>, requestDto));

    // ====================================================================
    // POST /v1/cache/delete —— 删除文件
    // ====================================================================
    ENDPOINT_INFO(deleteFile) {
        info->summary = "删除缓存中的文件";
        info->description =
            "接收 JSON Body，删除缓存目录中指定的文件。";
        info->addConsumes<oatpp::Object<my_api::dto::CacheFileRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST", "/v1/cache/delete", deleteFile,
             BODY_DTO(oatpp::Object<my_api::dto::CacheFileRequestDto>, requestDto));

    // ====================================================================
    // POST /v1/cache/list —— 获取所有文件列表
    // ====================================================================
    ENDPOINT_INFO(listFiles) {
        info->summary = "获取缓存目录中所有文件列表";
        info->description =
            "接收 JSON 请求体，可默认查询缓存根目录，\n"
            "也可指定缓存根目录内的子目录或绝对路径（仍需位于缓存根目录内）。\n"
            "返回目录下所有文件的元信息，包括文件名、大小、类型、最后修改时间。";
        info->addConsumes<oatpp::Object<my_api::dto::CacheListRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST", "/v1/cache/list", listFiles,
             BODY_DTO(oatpp::Object<my_api::dto::CacheListRequestDto>, requestDto));

    // ====================================================================
    // POST /v1/cache/create-folder —— 创建子目录
    // ====================================================================
    ENDPOINT_INFO(createFolder) {
        info->summary = "在缓存目录内创建子目录";
        info->description =
            "接收 JSON 请求体，在指定目录下创建新子目录。\n"
            "folder_path 可为空（表示缓存根目录），也可指定缓存根目录内的相对或绝对路径。\n"
            "new_folder_name 仅允许单层目录名，禁止越界访问。";
        info->addConsumes<oatpp::Object<my_api::dto::CacheCreateFolderRequestDto>>("application/json");
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_409, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("POST", "/v1/cache/create-folder", createFolder,
             BODY_DTO(oatpp::Object<my_api::dto::CacheCreateFolderRequestDto>, requestDto));

    // ====================================================================
    // GET /v1/cache/info —— 获取缓存配置信息
    // ====================================================================
    ENDPOINT_INFO(cacheInfo) {
        info->summary = "获取缓存配置信息";
        info->description =
            "返回当前文件缓存模块的配置信息，\n"
            "包括根目录路径、最大文件大小、最长保留时间等。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("GET", "/v1/cache/info", cacheInfo);

    // ====================================================================
    // GET /v1/cache/status —— 获取缓存运行状态
    // ====================================================================
    ENDPOINT_INFO(cacheStatus) {
        info->summary = "获取缓存运行状态";
        info->description =
            "返回当前文件缓存模块的运行状态，\n"
            "包括状态码和状态描述。";
        info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
        info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
    }
    ENDPOINT("GET", "/v1/cache/status", cacheStatus);
};

#include OATPP_CODEGEN_END(ApiController)

}  // namespace my_api::file_cache_api
