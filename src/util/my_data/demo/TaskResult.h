#pragma once
#include "Error.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <string>

namespace my_data {

/**
 * @brief Task 执行结果
 */
struct TaskResult {
  ErrorCode code{ErrorCode::Ok};
  std::string message{};
  TimestampMs started_at_ms{0};
  TimestampMs finished_at_ms{0};
  nlohmann::json output = nlohmann::json::object();

  /**
   * @brief 可读字符串（用于日志）
   */
  std::string toString() const;

  /**
   * @brief 转 json
   */
  nlohmann::json toJson() const;

  /**
   * @brief 从 json 解析
   */
  static TaskResult fromJson(const nlohmann::json& j);
};

} // namespace my_data