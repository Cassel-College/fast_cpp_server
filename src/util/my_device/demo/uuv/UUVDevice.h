#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>

#include "IDevice.h"
#include "MyData.h"
#include "MyLog.h"

// 依赖 my_control
#include "MyControl.h"              // my_control::MyControl 工厂
#include "IControl.h"               // my_control::IControl
#include "demo/Workflow.h"          // my_control::demo::Workflow
#include "TaskQueue.h"              // my_control::TaskQueue

namespace my_device::demo {

/**
 * @brief UUV 设备执行单元（demo）
 *
 * @details
 * - 内部持有：UUVControl + Workflow
 * - 外部注入：TaskQueue&（归 Edge）
 * - 维护状态：DeviceStatus（running_task_id / work_state / last_error 等）
 */
class UUVDevice final : public my_device::IDevice {
public:
  UUVDevice();
  UUVDevice(const nlohmann::json& cfg, std::string* err);
  ~UUVDevice() override;

  bool Init(const nlohmann::json& cfg, std::string* err) override;
  bool Start(my_control::TaskQueue& queue, std::atomic<bool>* estop_flag, std::string* err) override;

  void Stop() override;
  void Join() override;

  my_data::DeviceStatus GetStatusSnapshot() const override;

  my_data::DeviceId Id() const override { return device_id_; }
  std::string Type() const override { return "uuv"; }

private:
  void UpdateOnTaskStart(const my_data::Task& task);
  void UpdateOnTaskFinish(const my_data::Task& task, const my_data::TaskResult& r);

private:
  // 基础身份
  my_data::DeviceId device_id_{"uuv-unknown"};
  std::string device_name_{"uuv-demo"};

  // 控制器与工作流
  std::unique_ptr<my_control::IControl> control_;
  std::unique_ptr<my_control::demo::Workflow> workflow_;

  // 外部注入（仅保存指针用于日志/诊断；不负责生命周期）
  my_control::TaskQueue* queue_{nullptr};
  std::atomic<bool>* estop_flag_{nullptr};

  // 运行态快照
  mutable std::mutex status_mu_;
  my_data::DeviceStatus status_;
  std::optional<my_data::Task> running_task_snapshot_; // 可选，便于调试
};

} // namespace my_device::demo