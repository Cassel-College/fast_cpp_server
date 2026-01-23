#include "UUVCommandNormalizer.h"

#include "JsonUtil.h"
#include "demo/IdUtil.h"
#include "demo/TimeUtil.h"

namespace my_control::demo {

std::optional<my_data::Task> UUVCommandNormalizer::Normalize(const my_data::RawCommand& in,
                                                             const my_data::EdgeId& edge_id,
                                                             std::string* err) {
  MYLOG_INFO("[Normalizer:{}] Normalize 开始：command_id={}, source={}, received_at_ms={}",
             Name(), in.command_id, in.source, in.received_at_ms);

  if (!in.payload.is_object()) {
    if (err) *err = "payload is not a json object";
    MYLOG_ERROR("[Normalizer:{}] Normalize 失败：payload 非对象。raw={}", Name(), in.toString());
    return std::nullopt;
  }

  const auto& p = in.payload;

  std::string device_id = my_data::jsonutil::GetStringOr(p, "device_id", "");
  std::string capability = my_data::jsonutil::GetStringOr(p, "capability", "");
  std::string action = my_data::jsonutil::GetStringOr(p, "action", "");

  if (device_id.empty() || capability.empty() || action.empty()) {
    if (err) *err = "missing required fields: device_id/capability/action";
    MYLOG_ERROR("[Normalizer:{}] Normalize 失败：缺少字段 device_id/capability/action。payload={}",
                Name(), p.dump());
    return std::nullopt;
  }

  my_data::Task task;
  task.edge_id = edge_id;
  task.device_id = device_id;
  task.capability = capability;
  task.action = action;

  // params 可选
  auto pit = p.find("params");
  if (pit != p.end() && pit->is_object()) {
    task.params = *pit;
  } else {
    task.params = nlohmann::json::object();
  }

  // command_id：若外部未提供，则生成
  task.command_id = !in.command_id.empty() ? in.command_id : my_data::GenerateId("cmd");
  task.task_id = my_data::GenerateId("task");
  task.created_at_ms = my_data::NowMs();

  // 幂等字段：预留不启用
  task.idempotency_key = !in.idempotency_key.empty() ? in.idempotency_key : task.command_id;
  task.dedup_window_ms = in.dedup_window_ms;

  task.state = my_data::TaskState::Pending;

  MYLOG_INFO("[Normalizer:{}] Normalize 成功：{}", Name(), task.toString());
  return task;
}

} // namespace my_control::demo