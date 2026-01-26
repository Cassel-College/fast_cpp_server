#pragma once

#include <string>

#include "IControl.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_control::demo {

/**
 * @brief 分速传感器执行器（demo）
 *
 * 这里把“分速”理解为速度分量 (vx, vy, vz)，你也可以换成你真实定义。
 *
 * 支持能力：
 * - capability=wind
 *   - action=read
 */
class WindSensorDevice final : public my_control::IControl {
public:
  bool Init(const nlohmann::json& cfg, std::string* err) override;
  my_data::TaskResult DoTask(const my_data::Task& task) override;
  std::string Name() const override { return "WindSensorDevice"; }

private:
  std::string device_name_{"split-speed-sensor-demo"};
  int simulate_latency_ms_{50};
  double vx_base_{0.2};
  double vy_base_{0.1};
  double vz_base_{0.0};
  double noise_{0.05};
};

} // namespace my_control::demo