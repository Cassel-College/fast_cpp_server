#pragma once
#include "Types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace my_data {

/**
 * @brief 设备连接状态
 */
enum class DeviceConnState { Unknown = 0, Online = 1, Offline = 2 };

/**
 * @brief 设备工作状态
 */
enum class DeviceWorkState { Idle = 0, Busy = 1, Faulted = 2 };

/**
 * @brief Edge运行状态
 */
enum class EdgeRunState { Initializing = 0, Running = 1, EStop = 2, Degraded = 3 };

std::string ToString(DeviceConnState s);
std::string ToString(DeviceWorkState s);
std::string ToString(EdgeRunState s);

/**
 * @brief 单设备状态快照
 */
struct DeviceStatus {
  DeviceId device_id{};
  DeviceConnState conn_state{DeviceConnState::Unknown};
  DeviceWorkState work_state{DeviceWorkState::Idle};

  TaskId running_task_id{};
  TimestampMs last_task_at_ms{0};
  TimestampMs last_seen_at_ms{0};
  std::string last_error{};

  std::int64_t queue_depth{0};

  std::string toString() const;
  nlohmann::json toJson() const;
  static DeviceStatus fromJson(const nlohmann::json& j);
};

/**
 * @brief Edge状态快照
 */
struct EdgeStatus {
  EdgeId edge_id{};
  EdgeRunState run_state{EdgeRunState::Initializing};

  TimestampMs boot_at_ms{0};
  TimestampMs last_heartbeat_at_ms{0};

  bool estop_active{false};
  std::string estop_reason{};

  std::unordered_map<DeviceId, DeviceStatus> devices;

  std::int64_t tasks_pending_total{0};
  std::int64_t tasks_running_total{0};

  std::string version{"0.1.0"};

  std::string toString() const;
  nlohmann::json toJson() const;
  static EdgeStatus fromJson(const nlohmann::json& j);
};

} // namespace my_data