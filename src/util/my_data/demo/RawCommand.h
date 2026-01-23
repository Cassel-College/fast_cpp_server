#pragma once
#include "Types.h"
#include <nlohmann/json.hpp>
#include <string>

namespace my_data {

/**
 * @brief 原始控制命令（接收层输入）
 *
 * @note payload 使用 nlohmann::json，便于初始化和后续扩展字段。
 * @note received_at_ms 由接收层填写，表示接收时间戳（毫秒）。
 * 
 */
struct RawCommand {
  CommandId command_id{};
  std::string source{};
  nlohmann::json payload = nlohmann::json::object();
  TimestampMs received_at_ms{0};

  // 幂等（预留，不启用）
  std::string idempotency_key{};
  std::int64_t dedup_window_ms{0};

  std::string toString() const;
  nlohmann::json toJson() const;
  static RawCommand fromJson(const nlohmann::json& j);
};

} // namespace my_data