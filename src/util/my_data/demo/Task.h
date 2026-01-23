#pragma once
#include "TaskResult.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <string>

namespace my_data {

/**
 * @brief Task 状态（MVP）
 */
enum class TaskState {
  Pending = 0,
  Running = 1,
  Succeeded = 2,
  Failed = 3,
  Cancelled = 4,
};

/**
 * @brief Task（内部统一任务）
 *
 * @details
 * - 数据类中包含“幂等字段”，但 MVP 阶段不启用去重逻辑。
 * - params/output/policy 统一使用 nlohmann::json，初始化/扩展方便。
 */
struct Task {
  // 身份
  TaskId task_id{};
  CommandId command_id{};

  // 追踪（预留）
  std::string trace_id{};
  std::string span_id{};

  // 路由
  EdgeId edge_id{};
  DeviceId device_id{};

  // 能力与动作
  std::string capability{};
  std::string action{};
  nlohmann::json params = nlohmann::json::object();

  // 幂等（预留，不启用）
  std::string idempotency_key{};
  std::int64_t dedup_window_ms{0};

  // 调度（预留）
  int priority{0};
  TimestampMs created_at_ms{0};
  TimestampMs deadline_at_ms{0};
  nlohmann::json policy = nlohmann::json::object();

  // 运行态
  TaskState state{TaskState::Pending};
  TaskResult result{};

  std::string toString() const;
  nlohmann::json toJson() const;
  static Task fromJson(const nlohmann::json& j);
};

std::string ToString(TaskState s);

} // namespace my_data