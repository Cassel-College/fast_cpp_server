#include "UNAEdge.h"

#include "JsonUtil.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_edge::demo {

using namespace my_data;

UNAEdge::UNAEdge() : my_edge::BaseEdge("una") {
    // BaseEdge 构造里会打印日志
    MYLOG_INFO("[UNAEdge] 构造完成");
    MYLOG_INFO("[UNAEdge] 默认 MQTT 主题格式: {}", topic_fmt_);
    MYLOG_INFO("[UNAEdge] 默认 MQTT QoS: {}", qos_);
    MYLOG_INFO("[UNAEdge] 默认 MQTT retain: {}", retain_ ? "true" : "false");
}

UNAEdge::UNAEdge(const nlohmann::json& cfg, std::string* err) : UNAEdge() {
  this->ShowAnalyzeInitArgs(cfg);
  if (!Init(cfg, err)) {
    MYLOG_ERROR("[UNAEdge] 构造失败：Init 失败");
  } else {
    MYLOG_INFO("[UNAEdge] 构造成功");
  }
}

std::optional<my_data::Task> UNAEdge::NormalizeCommandLocked(const my_data::RawCommand& cmd,
                                                             const my_data::DeviceId& device_id,
                                                             const std::string& device_type,
                                                             std::string* err) {
  // 约定：self 也走 normalizer，这样外部下发 self task 的格式可以统一
  // device_type 在 BaseEdge 中对 self 固定写为 "self"
  // 你可以在 MyControl::CreateNormalizer("self") 里专门做一个 SelfNormalizer
  auto normalizer = my_control::MyControl::GetInstance().CreateNormalizer(device_type);
  if (!normalizer) {
    std::string e = "创建 normalizer 失败：type=" + device_type;
    MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
    if (err) *err = e;
    return std::nullopt;
  }

  std::string nerr;
  auto maybe_task = normalizer->Normalize(cmd, edge_id_, &nerr);
  if (!maybe_task.has_value()) {
    std::string e = nerr.empty() ? "Normalize 失败" : ("Normalize 失败：" + nerr);
    MYLOG_ERROR("[Edge:{}] device_id={}, type={}，{}", edge_id_, device_id, device_type, e);
    if (err) *err = e;
    return std::nullopt;
  }

  my_data::Task t = *maybe_task;

  // 防御式校验：确保 task.device_id 与路由一致（避免 normalizer 输出脏数据）
  if (t.device_id.empty()) {
    t.device_id = device_id;
  }
  if (t.device_id != device_id) {
    std::string e = "Normalize 输出的 task.device_id 与请求 device_id 不一致："
                    "task.device_id=" + t.device_id + ", req.device_id=" + device_id;
    MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
    if (err) *err = e;
    return std::nullopt;
  }

  MYLOG_INFO("[Edge:{}] Normalize 成功：device_id={}, type={}, task_id={}",
             edge_id_, device_id, device_type, t.task_id);
  return t;
}

void UNAEdge::ExecuteOtherTaskLocked(const my_data::Task& task) {
  // 先记录一条总日志，便于排查
  MYLOG_INFO("[Edge:{}] UNAEdge 执行 self task：task_id={}, capability={}, action={}, params={}",
             edge_id_, task.task_id, task.capability, task.action, task.params.dump());

  // 示例：实现一些内置 self task

  // 未识别任务：交给 BaseEdge 默认实现（保持兼容）
  my_edge::BaseEdge::ExecuteOtherTaskLocked(task);
}

void UNAEdge::ExecuteSelfTaskLocked() {
  // 先记录一条总日志，便于排查
  MYLOG_INFO("[Edge:{}] UNAEdge 执行 self task：task_id={}, capability={}, action={}, params={}",
             edge_id_, self_task.task_id, self_task.capability, self_task.action, self_task.params.dump());

  // 示例：实现一些内置 self task

  // 未识别任务：交给 BaseEdge 默认实现（保持兼容）
  my_edge::BaseEdge::ExecuteSelfTaskLocked();
}

void UNAEdge::SendHeatbeatByMQTT() {
    {
        my_mqtt::MqttService& mqtt_service = my_mqtt::MqttService::GetInstance();
    }
    if (!my_mqtt::MqttService::GetInstance().IsRunning()) {
        MYLOG_WARN("[Edge:{}] MQTT 发布器未初始化，无法发送心跳", edge_id_);
        return;
    }
    std::string topic = topic_fmt_;
    // 替换 {source} 占位符
    size_t pos = topic.find("{source}");
    if (pos != std::string::npos) {
        topic.replace(pos, std::string("{source}").length(), edge_id_);
    }
    nlohmann::json payload = {{"edge_id", edge_id_}, {"timestamp", std::time(nullptr)}};
    my_mqtt::MqttService::GetInstance().GetPublisher()->Publish(topic, payload.dump(), qos_, retain_);
    MYLOG_INFO("[Edge:{}] 通过 MQTT 发布心跳: topic={}, payload={}", edge_id_, topic, payload.dump());
}

void UNAEdge::ReportHeartbeatLocked() {

  // 再发送 MQTT 心跳
  SendHeatbeatByMQTT();
}

} // namespace my_edge::demo