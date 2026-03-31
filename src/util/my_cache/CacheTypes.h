#pragma once

/**
 * @file CacheTypes.h
 * @brief my_cache 模块公共类型定义
 *
 * 包含：
 *   - CacheErrorCode 错误码枚举
 *   - CacheErrorCodeToString 错误码转字符串
 *   - CacheResult<T> 通用返回值包装类
 */

#include <string>
#include <utility>

namespace my_cache {

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
