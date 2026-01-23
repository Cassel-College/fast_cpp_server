#include "Status.h"
#include "../JsonUtil.h"
#include <sstream>

namespace my_data {

std::string ToString(DeviceConnState s) {
  switch (s) {
    case DeviceConnState::Unknown: return "Unknown";
    case DeviceConnState::Online: return "Online";
    case DeviceConnState::Offline: return "Offline";
    default: return "UnknownDeviceConnState";
  }
}
std::string ToString(DeviceWorkState s) {
  switch (s) {
    case DeviceWorkState::Idle: return "Idle";
    case DeviceWorkState::Busy: return "Busy";
    case DeviceWorkState::Faulted: return "Faulted";
    default: return "UnknownDeviceWorkState";
  }
}
std::string ToString(EdgeRunState s) {
  switch (s) {
    case EdgeRunState::Initializing: return "Initializing";
    case EdgeRunState::Running: return "Running";
    case EdgeRunState::EStop: return "EStop";
    case EdgeRunState::Degraded: return "Degraded";
    default: return "UnknownEdgeRunState";
  }
}

std::string DeviceStatus::toString() const {
  std::ostringstream oss;
  oss << "DeviceStatus{device_id=" << device_id
      << ", conn_state=" << my_data::ToString(conn_state)
      << ", work_state=" << my_data::ToString(work_state)
      << ", running_task_id=" << running_task_id
      << ", last_task_at_ms=" << last_task_at_ms
      << ", last_seen_at_ms=" << last_seen_at_ms
      << ", last_error=" << last_error
      << ", queue_depth=" << queue_depth
      << "}";
  return oss.str();
}

nlohmann::json DeviceStatus::toJson() const {
  nlohmann::json j = nlohmann::json::object();
  j["device_id"] = device_id;
  j["conn_state"] = static_cast<int>(conn_state);
  j["work_state"] = static_cast<int>(work_state);
  j["running_task_id"] = running_task_id;
  j["last_task_at_ms"] = last_task_at_ms;
  j["last_seen_at_ms"] = last_seen_at_ms;
  j["last_error"] = last_error;
  j["queue_depth"] = queue_depth;
  return j;
}

DeviceStatus DeviceStatus::fromJson(const nlohmann::json& j) {
  DeviceStatus s;
  s.device_id = my_data::jsonutil::GetStringOr(j, "device_id", "");
  s.conn_state = static_cast<DeviceConnState>(my_data::jsonutil::GetIntOr(j, "conn_state", 0));
  s.work_state = static_cast<DeviceWorkState>(my_data::jsonutil::GetIntOr(j, "work_state", 0));
  s.running_task_id = my_data::jsonutil::GetStringOr(j, "running_task_id", "");
  s.last_task_at_ms = my_data::jsonutil::GetInt64Or(j, "last_task_at_ms", 0);
  s.last_seen_at_ms = my_data::jsonutil::GetInt64Or(j, "last_seen_at_ms", 0);
  s.last_error = my_data::jsonutil::GetStringOr(j, "last_error", "");
  s.queue_depth = my_data::jsonutil::GetInt64Or(j, "queue_depth", 0);
  return s;
}

std::string EdgeStatus::toString() const {
  std::ostringstream oss;
  oss << "EdgeStatus{edge_id=" << edge_id
      << ", run_state=" << my_data::ToString(run_state)
      << ", boot_at_ms=" << boot_at_ms
      << ", last_heartbeat_at_ms=" << last_heartbeat_at_ms
      << ", estop_active=" << (estop_active ? "true" : "false")
      << ", estop_reason=" << estop_reason
      << ", tasks_pending_total=" << tasks_pending_total
      << ", tasks_running_total=" << tasks_running_total
      << ", version=" << version
      << ", devices=" << devices.size()
      << "}";
  return oss.str();
}

nlohmann::json EdgeStatus::toJson() const {
  nlohmann::json j = nlohmann::json::object();
  j["edge_id"] = edge_id;
  j["run_state"] = static_cast<int>(run_state);
  j["boot_at_ms"] = boot_at_ms;
  j["last_heartbeat_at_ms"] = last_heartbeat_at_ms;
  j["estop_active"] = estop_active;
  j["estop_reason"] = estop_reason;
  j["tasks_pending_total"] = tasks_pending_total;
  j["tasks_running_total"] = tasks_running_total;
  j["version"] = version;

  nlohmann::json dev = nlohmann::json::object();
  for (const auto& [id, st] : devices) {
    dev[id] = st.toJson();
  }
  j["devices"] = dev;
  return j;
}

EdgeStatus EdgeStatus::fromJson(const nlohmann::json& j) {
  EdgeStatus s;
  s.edge_id = my_data::jsonutil::GetStringOr(j, "edge_id", "");
  s.run_state = static_cast<EdgeRunState>(my_data::jsonutil::GetIntOr(j, "run_state", 0));
  s.boot_at_ms = my_data::jsonutil::GetInt64Or(j, "boot_at_ms", 0);
  s.last_heartbeat_at_ms = my_data::jsonutil::GetInt64Or(j, "last_heartbeat_at_ms", 0);

  auto it = j.find("estop_active");
  if (it != j.end() && it->is_boolean()) s.estop_active = it->get<bool>();
  s.estop_reason = my_data::jsonutil::GetStringOr(j, "estop_reason", "");

  s.tasks_pending_total = my_data::jsonutil::GetInt64Or(j, "tasks_pending_total", 0);
  s.tasks_running_total = my_data::jsonutil::GetInt64Or(j, "tasks_running_total", 0);
  s.version = my_data::jsonutil::GetStringOr(j, "version", "0.1.0");

  auto dit = j.find("devices");
  if (dit != j.end() && dit->is_object()) {
    for (auto it2 = dit->begin(); it2 != dit->end(); ++it2) {
      if (it2.value().is_object()) {
        s.devices[it2.key()] = DeviceStatus::fromJson(it2.value());
      }
    }
  }
  return s;
}

} // namespace my_data