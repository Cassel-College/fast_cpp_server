#pragma once

/**
 * @file FileApiHelpers.h
 * @brief FileApiController 辅助函数（参数校验、错误码映射）
 *
 * 将业务逻辑与网络传输层解耦，便于独立单元测试。
 */

#include "CacheTypes.h"

#include <string>
#include <sstream>

namespace my_api::file_cache_api {

/**
 * @brief 校验文件名合法性
 *
 * 规则：
 *   - 不得为空
 *   - 不得包含 '\0' 空字符
 *   - 不得包含 '\\'（反斜杠，Windows 风格路径）
 *   - 不得以 '/' 开头（绝对路径）
 *   - 不得包含 ".." 路径组件（路径穿越）
 *   - 长度不超过 1024 字符
 *
 * @param filename 待校验的文件名
 * @param[out] err_msg 校验失败时的错误描述
 * @return true 合法，false 非法
 */
inline bool ValidateFilename(const std::string& filename, std::string& err_msg) {
    if (filename.empty()) {
        err_msg = "filename 不能为空";
        return false;
    }

    if (filename.size() > 1024) {
        err_msg = "filename 长度超过 1024 字符限制";
        return false;
    }

    if (filename.find('\0') != std::string::npos) {
        err_msg = "filename 包含非法空字符";
        return false;
    }

    if (filename.find('\\') != std::string::npos) {
        err_msg = "filename 不允许包含反斜杠 '\\'";
        return false;
    }

    if (filename[0] == '/') {
        err_msg = "filename 不允许以 '/' 开头";
        return false;
    }

    // 路径穿越检查：检测 ".." 组件
    std::string segment;
    std::istringstream stream(filename);
    while (std::getline(stream, segment, '/')) {
        if (segment == "..") {
            err_msg = "filename 包含非法路径穿越组件 '..'";
            return false;
        }
    }

    return true;
}

/**
 * @brief 将 CacheErrorCode 映射为 HTTP 状态码
 * @param code 缓存错误码
 * @return 对应的 HTTP 状态码整数
 */
inline int CacheErrorToHttpCode(my_cache::CacheErrorCode code) {
    switch (code) {
        case my_cache::CacheErrorCode::Ok:               return 200;
        case my_cache::CacheErrorCode::NotInitialized:   return 500;
        case my_cache::CacheErrorCode::PathTraversal:    return 403;
        case my_cache::CacheErrorCode::FileNotFound:     return 404;
        case my_cache::CacheErrorCode::FileAlreadyExists:return 409;
        case my_cache::CacheErrorCode::IoError:          return 500;
        case my_cache::CacheErrorCode::InvalidArgument:  return 400;
        case my_cache::CacheErrorCode::CreateDirFailed:  return 500;
        case my_cache::CacheErrorCode::FileTooLarge:     return 413;
        default:                                         return 500;
    }
}

}  // namespace my_api::file_cache_api
