#pragma once

/**
 * @file CacheTypes.h
 * @brief my_cache 模块公共类型定义
 *
 * 包含：
 *   - CacheStatus 模块状态枚举
 *   - CacheConfig 配置结构体（由 Init 的 JSON 参数解析得到）
 *   - FileInfo 文件元信息结构体
 *   - CacheErrorCode 错误码枚举
 *   - CacheErrorCodeToString 错误码转字符串
 *   - CacheResult<T> 通用返回值包装类
 */

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace my_cache {

// ============================================================================
// 模块状态枚举
// ============================================================================

/**
 * @brief 缓存模块运行状态
 */
enum class CacheStatus {
    NotInitialized = 0,  // 尚未初始化（构造后默认状态）
    Running,             // 正常运行中
    Error,               // 初始化失败
};

/**
 * @brief 将状态码转为可读字符串
 */
const char* CacheStatusToString(CacheStatus status);

// ============================================================================
// 配置结构体
// ============================================================================

/**
 * @brief 缓存模块配置（由 Init 时的 JSON 解析得到）
 *
 * JSON 示例：
 * @code{.json}
 * {
 *     "root_path": "/data/cache",
 *     "max_file_size": 104857600,
 *     "max_retention_seconds": 86400
 * }
 * @endcode
 */
struct CacheConfig {
    std::string root_path;                  ///< 缓存根目录路径（必填）
    uint64_t max_file_size = 0;             ///< 单个文件最大大小（字节），0 表示不限制
    int64_t max_retention_seconds = 0;      ///< 文件最长保留时间（秒），0 表示不限制，-1 表示永久有效
};

// ============================================================================
// 文件元信息
// ============================================================================

/**
 * @brief 文件元信息结构体
 */
struct FileInfo {
    std::string name;          ///< 相对路径名（如 "sub/file.txt"）
    uint64_t size = 0;         ///< 文件大小（字节）
    std::string type;          ///< 文件扩展名（如 "txt"、"bin"，无扩展名为空）
    std::string modified_at;   ///< 最后修改时间（ISO 8601 格式，如 "2026-03-31T14:30:00"）
};

// ============================================================================
// 错误码枚举
// ============================================================================

/**
 * @brief 缓存操作错误码
 */
enum class CacheErrorCode {
    Ok = 0,              // 操作成功
    NotInitialized,      // 模块未初始化
    PathTraversal,       // 路径穿越攻击（请求路径超出根目录范围）
    FileNotFound,        // 文件不存在
    FileAlreadyExists,   // 文件已存在
    IoError,             // 文件读写错误
    InvalidArgument,     // 参数非法（如文件名为空）
    CreateDirFailed,     // 创建目录失败
    FileTooLarge,        // 文件超过最大大小限制
};

/**
 * @brief 将错误码转为可读字符串
 */
const char* CacheErrorCodeToString(CacheErrorCode code);

// ============================================================================
// 返回值包装类
// ============================================================================

/**
 * @brief 通用返回值类型，包含错误码和可选的数据
 * @tparam T 返回数据的类型
 */
template <typename T>
struct CacheResult {
    CacheErrorCode code;
    T value;

    /// 判断操作是否成功
    bool Ok() const { return code == CacheErrorCode::Ok; }

    /// 快速构造成功结果
    static CacheResult Success(T val) {
        return {CacheErrorCode::Ok, std::move(val)};
    }

    /// 快速构造失败结果
    static CacheResult Fail(CacheErrorCode err, T val = T{}) {
        return {err, std::move(val)};
    }
};

/**
 * @brief 无数据返回的特化版本（仅表示成功/失败）
 */
template <>
struct CacheResult<void> {
    CacheErrorCode code;

    bool Ok() const { return code == CacheErrorCode::Ok; }

    static CacheResult Success() {
        return {CacheErrorCode::Ok};
    }

    static CacheResult Fail(CacheErrorCode err) {
        return {err};
    }
};

}  // namespace my_cache
