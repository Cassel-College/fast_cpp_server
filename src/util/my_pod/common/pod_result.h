#pragma once

/**
 * @file pod_result.h
 * @brief 吊舱模块统一结果封装
 * 
 * 所有吊舱操作返回统一的 PodResult<T> 结构，包含错误码、描述和数据。
 */

#include "pod_errors.h"
#include <string>
#include <optional>

namespace PodModule {

/**
 * @brief 统一操作结果模板
 * 
 * @tparam T 返回数据类型，默认为 void 特化
 */
template<typename T = void>
struct PodResult {
    PodErrorCode    code = PodErrorCode::SUCCESS;   // 错误码
    std::string     message;                         // 描述信息
    std::optional<T> data;                           // 返回数据

    /** @brief 判断操作是否成功 */
    bool isSuccess() const { return code == PodErrorCode::SUCCESS; }

    /** @brief 构造成功结果 */
    static PodResult success(const T& value, const std::string& msg = "成功") {
        return PodResult{PodErrorCode::SUCCESS, msg, value};
    }

    /** @brief 构造失败结果 */
    static PodResult fail(PodErrorCode err, const std::string& msg = "") {
        std::string desc = msg.empty() ? podErrorCodeToString(err) : msg;
        return PodResult{err, desc, std::nullopt};
    }
};

/**
 * @brief void 特化：无返回数据的操作结果
 */
template<>
struct PodResult<void> {
    PodErrorCode code = PodErrorCode::SUCCESS;
    std::string  message;

    bool isSuccess() const { return code == PodErrorCode::SUCCESS; }

    static PodResult success(const std::string& msg = "成功") {
        return PodResult{PodErrorCode::SUCCESS, msg};
    }

    static PodResult fail(PodErrorCode err, const std::string& msg = "") {
        std::string desc = msg.empty() ? podErrorCodeToString(err) : msg;
        return PodResult{err, desc};
    }
};

} // namespace PodModule
