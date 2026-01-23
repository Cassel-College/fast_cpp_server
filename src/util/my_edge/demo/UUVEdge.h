#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "IEdge.h"
#include "MyData.h"
#include "MyLog.h"



#include "IDevice.h"


#include "ICommandNormalizer.h"
#include "TaskQueue.h"

using namespace my_data;
using namespace my_control;


namespace my_edge::demo {

/**
 * @brief UUVEdge：demo Edge Runtime
 *
 * @details
 * - Init/Start 分离
 * - 每个 device_id：
 *   - Edge 持有 TaskQueue 实例（unique_ptr）
 *   - Edge 持有 Device 实例（unique_ptr）
 * - Submit:
 *   - 从 payload 顶层读 device_id（强制要求）
 *   - 找到 device type
 *   - 按 type 选择 normalizer（ICommandNormalizer）
 *   - Normalize 得 Task 并 Push 到队列
 * - EStop 默认拒绝入队（可 cfg allow_queue_when_estop）
 */
class UUVEdge final : public my_edge::IEdge {
public:
  UUVEdge();
  ~UUVEdge() override;

  bool Init(const nlohmann::json& cfg, std::string* err) override;
  bool Start(std::string* err) override;
  SubmitResult Submit(const my_data::RawCommand& cmd) override;

  my_data::EdgeStatus GetStatusSnapshot() const override;
  void SetEStop(bool active, const std::string& reason) override;
  void Shutdown() override;

  my_data::EdgeId Id() const override { return edge_id_; }

private:
  enum class RunState { Initializing, Ready, Running, Stopping, Stopped };

  static std::string ToString(RunState s);

  SubmitResult MakeResult(SubmitCode code, const std::string& msg,
                          const my_data::RawCommand& cmd,
                          const my_data::DeviceId& device_id = "",
                          const my_data::TaskId& task_id = "",
                          std::int64_t queue_size_after = 0) const;

  bool EnsureNormalizerForTypeLocked(const std::string& type, std::string* err);

private:
  mutable std::shared_mutex rw_mutex_;

  // 静态信息（Init 固化）
  my_data::EdgeId edge_id_{"edge-unknown"};
  std::string version_{"0.1.0"};
  my_data::TimestampMs boot_at_ms_{0};

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
  std::unordered_map<my_data::DeviceId, std::unique_ptr<my_device::IDevice>> devices_;

  // 保存 cfg（可选：为了调试/查看）
  nlohmann::json cfg_;
};

} // namespace my_edge::demo