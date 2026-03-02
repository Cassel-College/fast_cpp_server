#include "UNAEdge.h"

#include "CS-Y2536.pb.h"
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
    InitMQTTRoute();
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
    CSY2536::agent_to_centre_info_heartbeat_pb heartbeat;

    heartbeat.set_agent_id(1);
    heartbeat.set_agent_type(::CSY2536::AGENT_TYPE::AGENT_TYPE_UUV);
    heartbeat.set_status(::CSY2536::AGENT_STATUS_TYPE::AGENT_STATUS_RESCUE_ACTIVE);
    heartbeat.set_error_code(0);

    CSY2536::Poses temp_pos;
    temp_pos.set_pitch(45);
    temp_pos.set_yaw(10);  
    temp_pos.set_roll(1);
    heartbeat.mutable_po()->CopyFrom(temp_pos);

    heartbeat.set_battery(100);
    heartbeat.set_endurance(3600);

    CSY2536::Poses temp_po;
    temp_po.set_pitch(45);
    temp_po.set_yaw(10);
    temp_po.set_roll(1);
    heartbeat.mutable_po()->CopyFrom(temp_po);

    std::string payload_mqtt;
    if (!heartbeat.SerializeToString(&payload_mqtt)) {
       MYLOG_ERROR("[Edge:{}] 序列化心跳失败", edge_id_);
        return;
    }

    nlohmann::json payload = {{"edge_id", edge_id_}, {"timestamp", std::time(nullptr)}};
    // my_mqtt::MqttService::GetInstance().GetPublisher()->Publish(topic, payload.dump(), qos_, retain_);
    my_mqtt::MqttService::GetInstance().GetPublisher()->Publish(topic, payload_mqtt, qos_, retain_);
    MYLOG_INFO("[Edge:{}] 通过 MQTT 发布心跳: topic={}, payload={}", edge_id_, topic, payload.dump());
}

void UNAEdge::ReportHeartbeatLocked() {
  // 发送 MQTT 心跳
  SendHeatbeatByMQTT();
}

void UNAEdge::InitMQTTRoute() {
    MYLOG_INFO("[Edge:{}] 初始化 MQTT 路由", edge_id_);
    my_mqtt::MqttService& mqtt_service = my_mqtt::MqttService::GetInstance();

    mqtt_service.AddRoute("yingji/to_agent/server_messages", [](const std::string& topic, const std::string& payload) {
        MYLOG_INFO("MQTTComm 收到操作请求，Topic: {}, Payload: {}", topic, payload);
        // 处理操作请求的逻辑
    });

    mqtt_service.AddRoute("hello/a", [](const std::string& topic, const std::string& payload) {
        MYLOG_INFO("MQTTComm 收到操作请求，Topic: {}, Payload: {}", topic, payload);
        // 处理操作请求的逻辑
    });

    mqtt_service.AddRoute("hello/b", [](const std::string& topic, const std::string& payload) {
        MYLOG_INFO("MQTTComm 收到操作请求，Topic: {}, Payload: {}", topic, payload);
        // 处理操作请求的逻辑
    });

    MYLOG_INFO("[Edge:{}] MQTT 路由初始化完成", edge_id_);
    return;
}

} // namespace my_edge::demo