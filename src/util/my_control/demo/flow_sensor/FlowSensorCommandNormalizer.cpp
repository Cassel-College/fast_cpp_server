#include "FlowSensorCommandNormalizer.h"

#include "JsonUtil.h"
#include "MyLog.h"
#include "demo/IdUtil.h"
#include "demo/TimeUtil.h"

namespace my_control::demo {

std::optional<my_data::Task> FlowSensorCommandNormalizer::Normalize(const my_data::RawCommand& in,
                                                                    const my_data::EdgeId& edge_id,
                                                                    std::string* err) {
  MYLOG_INFO("[Normalizer:FlowSensor] Normalize begin: command_id={}, source={}, received_at_ms={}",
             in.command_id, in.source, in.received_at_ms);

  if (!in.payload.is_object()) {
    if (err) *err = "payload is not a json object";
    MYLOG_ERROR("[Normalizer:FlowSensor] payload not object. raw={}", in.toString());
    return std::nullopt;
  }

  const auto& p = in.payload;

  std::string device_id = my_data::jsonutil::GetStringOr(p, "device_id", "");
  if (device_id.empty()) {
    if (err) *err = "missing required field: device_id";
    MYLOG_ERROR("[Normalizer:FlowSensor] missing device_id. payload={}", p.dump());
    return std::nullopt;
  }

  std::string action = my_data::jsonutil::GetStringOr(p, "action", "read");

  my_data::Task task;
  task.edge_id = edge_id;
  task.device_id = device_id;

  task.capability = "flow_speed";
  task.action = action;

  auto pit = p.find("params");
  if (pit != p.end() && pit->is_object()) {
    task.params = *pit;
  } else {
    task.params = nlohmann::json::object();
  }

  task.command_id = !in.command_id.empty() ? in.command_id : my_data::GenerateId("cmd");
  task.task_id = my_data::GenerateId("task");
  task.created_at_ms = my_data::NowMs();

  task.idempotency_key = !in.idempotency_key.empty() ? in.idempotency_key : task.command_id;
  task.dedup_window_ms = in.dedup_window_ms;

  task.state = my_data::TaskState::Pending;

  MYLOG_INFO("[Normalizer:FlowSensor] Normalize ok: {}", task.toString());
  return task;
}

} // namespace my_control::demo