#pragma once

#include <string>

#include "IControl.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_control::demo {

/**
 * @brief 流速传感器执行器（demo）
 *
 * 支持能力：
 * - capability=flow_speed
 *   - action=read
 */
class FlowSensorControl final : public my_control::IControl {
public:
  bool Init(const nlohmann::json& cfg, std::string* err) override;
  my_data::TaskResult DoTask(const my_data::Task& task) override;
  std::string Name() const override { return "FlowSensorControl"; }

private:
  std::string device_name_{"flow-sensor-demo"};
  int simulate_latency_ms_{50};
  double flow_base_mps_{0.8};
  double flow_noise_mps_{0.1};
};

} // namespace my_control::demo