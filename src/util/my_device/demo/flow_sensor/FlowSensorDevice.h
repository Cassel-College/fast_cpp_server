#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>

#include "IDevice.h"
#include "MyData.h"
#include "MyLog.h"

#include "MyControl.h"
#include "IControl.h"
#include "demo/Workflow.h"
#include "TaskQueue.h"

namespace my_device::demo {

class FlowSensorDevice final : public my_device::IDevice {
public:
  FlowSensorDevice();
  FlowSensorDevice(const nlohmann::json& cfg, std::string* err);
  ~FlowSensorDevice() override;

  bool Init(const nlohmann::json& cfg, std::string* err) override;
  bool Start(my_control::TaskQueue& queue, std::atomic<bool>* estop_flag, std::string* err) override;

  void Stop() override;
  void Join() override;

  my_data::DeviceStatus GetStatusSnapshot() const override;

  my_data::DeviceId Id() const override { return device_id_; }
  std::string Type() const override { return "flow_sensor"; }

private:
  void UpdateOnTaskStart(const my_data::Task& task);
  void UpdateOnTaskFinish(const my_data::Task& task, const my_data::TaskResult& r);

private:
  my_data::DeviceId device_id_{"flow-unknown"};
  std::string device_name_{"flow-sensor-demo"};

  std::unique_ptr<my_control::IControl> control_;
  std::unique_ptr<my_control::demo::Workflow> workflow_;

  my_control::TaskQueue* queue_{nullptr};
  std::atomic<bool>* estop_flag_{nullptr};

  mutable std::mutex status_mu_;
  my_data::DeviceStatus status_;
  std::optional<my_data::Task> running_task_snapshot_;
};

} // namespace my_device::demo