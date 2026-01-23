#include "Task.h"
#include "../JsonUtil.h"
#include <sstream>

namespace my_data {

std::string ToString(TaskState s) {
  switch (s) {
    case TaskState::Pending: return "Pending";
    case TaskState::Running: return "Running";
    case TaskState::Succeeded: return "Succeeded";
    case TaskState::Failed: return "Failed";
    case TaskState::Cancelled: return "Cancelled";
    default: return "UnknownTaskState";
  }
}

std::string Task::toString() const {
  std::ostringstream oss;
  oss << "Task{task_id=" << task_id
      << ", command_id=" << command_id
      << ", edge_id=" << edge_id
      << ", device_id=" << device_id
      << ", capability=" << capability
      << ", action=" << action
      << ", params=" << params.dump()
      << ", idempotency_key=" << idempotency_key
      << ", dedup_window_ms=" << dedup_window_ms
      << ", priority=" << priority
      << ", created_at_ms=" << created_at_ms
      << ", deadline_at_ms=" << deadline_at_ms
      << ", policy=" << policy.dump()
      << ", state=" << my_data::ToString(state)
      << ", result=" << result.toString()
      << "}";
  return oss.str();
}

nlohmann::json Task::toJson() const {
  nlohmann::json j = nlohmann::json::object();
  j["task_id"] = task_id;
  j["command_id"] = command_id;
  j["trace_id"] = trace_id;
  j["span_id"] = span_id;
  j["edge_id"] = edge_id;
  j["device_id"] = device_id;
  j["capability"] = capability;
  j["action"] = action;
  j["params"] = params;
  j["idempotency_key"] = idempotency_key;
  j["dedup_window_ms"] = dedup_window_ms;
  j["priority"] = priority;
  j["created_at_ms"] = created_at_ms;
  j["deadline_at_ms"] = deadline_at_ms;
  j["policy"] = policy;
  j["state"] = static_cast<int>(state);
  j["result"] = result.toJson();
  return j;
}

Task Task::fromJson(const nlohmann::json& j) {
  Task t;
  t.task_id = my_data::jsonutil::GetStringOr(j, "task_id", "");
  t.command_id = my_data::jsonutil::GetStringOr(j, "command_id", "");
  t.trace_id = my_data::jsonutil::GetStringOr(j, "trace_id", "");
  t.span_id = my_data::jsonutil::GetStringOr(j, "span_id", "");
  t.edge_id = my_data::jsonutil::GetStringOr(j, "edge_id", "");
  t.device_id = my_data::jsonutil::GetStringOr(j, "device_id", "");
  t.capability = my_data::jsonutil::GetStringOr(j, "capability", "");
  t.action = my_data::jsonutil::GetStringOr(j, "action", "");

  auto pit = j.find("params");
  if (pit != j.end() && pit->is_object()) t.params = *pit;

  t.idempotency_key = my_data::jsonutil::GetStringOr(j, "idempotency_key", "");
  t.dedup_window_ms = my_data::jsonutil::GetInt64Or(j, "dedup_window_ms", 0);

  t.priority = my_data::jsonutil::GetIntOr(j, "priority", 0);
  t.created_at_ms = my_data::jsonutil::GetInt64Or(j, "created_at_ms", 0);
  t.deadline_at_ms = my_data::jsonutil::GetInt64Or(j, "deadline_at_ms", 0);

  auto pol = j.find("policy");
  if (pol != j.end() && pol->is_object()) t.policy = *pol;

  int state_i = my_data::jsonutil::GetIntOr(j, "state", static_cast<int>(TaskState::Pending));
  t.state = static_cast<TaskState>(state_i);

  auto rit = j.find("result");
  if (rit != j.end() && rit->is_object()) t.result = TaskResult::fromJson(*rit);

  return t;
}

} // namespace my_data