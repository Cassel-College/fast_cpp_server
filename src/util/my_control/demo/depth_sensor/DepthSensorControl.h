#pragma once

#include <string>

#include "IControl.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_control::demo {

/**
 * @brief 水深传感器执行器（demo）
 *
 * 支持能力：
 * - capability=water_depth
 *   - action=read    : 读取水深
 *   - action=calibrate : 校准（demo）
 */
class DepthSensorControl final : public my_control::IControl {
public:
  bool Init(const nlohmann::json& cfg, std::string* err) override;
  my_data::TaskResult DoTask(const my_data::Task& task) override;
  std::string Name() const override { return "DepthSensorControl"; }

private:
  std::string device_name_{"depth-sensor-demo"};
  int simulate_latency_ms_{50};
  double depth_base_m_{10.0};
  double depth_noise_m_{0.2};
};

} // namespace my_control::demo