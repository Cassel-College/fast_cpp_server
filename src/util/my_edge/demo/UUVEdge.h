#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "IEdge.h"
#include "MyData.h"
#include "MyLog.h"


#include "IDevice.h"


#include "ICommandNormalizer.h"
#include "TaskQueue.h"

namespace my_edge::demo {

class UUVEdge final : public my_edge::IEdge {
public:
  UUVEdge();
  UUVEdge(const nlohmann::json& cfg, std::string* err=nullptr);
  ~UUVEdge() override;

  bool Init(const nlohmann::json& cfg, std::string* err) override;
  bool Start(std::string* err) override;
  SubmitResult Submit(const my_data::RawCommand& cmd) override;

  my_data::EdgeStatus GetStatusSnapshot() const override;
  void SetEStop(bool active, const std::string& reason) override;
  void Shutdown() override;
  nlohmann::json DumpInternalInfo() const override;
  // 解释 Init 入参并输出到日志（实现 IEdge 接口）
  void ShowAnalyzeInitArgs(const nlohmann::json& cfg) const override;

  my_data::EdgeId Id() const override { return edge_id_; }

  std::string EdgeType() const override { return edge_type_; }

private:
  enum class RunState { Initializing, Ready, Running, Stopping, Stopped };
  static std::string ToString(RunState s);

  SubmitResult MakeResult(SubmitCode code, const std::string& msg,
                          const my_data::RawCommand& cmd,
                          const my_data::DeviceId& device_id = "",
                          const my_data::TaskId& task_id = "",
                          std::int64_t queue_size_after = 0) const;

  /**
   * @brief 
   * 
   * @param {type} type 
   * @param {type} err 
   * @return true 
   * @return false 
   */
  bool EnsureNormalizerForTypeLocked(const std::string& type, std::string* err);

  // ---- status snapshot thread ----
  void StartStatusSnapshotThreadLocked();
  void StopStatusSnapshotThreadLocked();
  void StatusSnapshotLoop();

private:
  mutable std::shared_mutex rw_mutex_;

  // 静态信息（Init 固化）
  my_data::EdgeId edge_id_{"edge-unknown"};
  std::string version_{"0.1.0"};
  my_data::TimestampMs boot_at_ms_{0};
  std::string edge_type_{"uuv"};

  // 运行态
  std::atomic<RunState> run_state_{RunState::Initializing};

  // EStop
  std::atomic<bool> estop_{false};
  mutable std::mutex estop_mu_;
  std::string estop_reason_{};
  bool allow_queue_when_estop_{false};

  // device_id -> type
  std::unordered_map<my_data::DeviceId, std::string> device_type_by_id_;

  // type -> normalizer（复用）
  std::unordered_map<std::string, std::unique_ptr<my_control::ICommandNormalizer>> normalizers_by_type_;

  // device_id -> queue/device（实例归 Edge 持有）
  std::unordered_map<my_data::DeviceId, std::unique_ptr<my_control::TaskQueue>> queues_;
  std::unordered_map<my_data::DeviceId, std::unique_ptr<my_control::TaskQueue>> history_queues_;
  std::unordered_map<my_data::DeviceId, std::unique_ptr<my_device::IDevice>> devices_;

  // 保存 cfg（调试）
  nlohmann::json cfg_;

  // ---- snapshot thread config/state ----
  bool status_snapshot_enable_{false};
  int status_snapshot_interval_ms_{5000};
  std::atomic<bool> snapshot_stop_{false};
  std::thread snapshot_thread_;
};

} // namespace my_edge::demo