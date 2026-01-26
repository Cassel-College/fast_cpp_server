#pragma once

#include <string>

#include "IControl.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_control::demo {

/**
 * @brief UUV 执行器（demo）
 *
 * @details
 * - 这是 demo 实现：用 sleep 模拟真实驱动执行
 * - 后续接入真实设备驱动时，只需替换这里的 DoTask 实现
 */
class UUVControl final : public my_control::IControl {
public:
  bool Init(const nlohmann::json& cfg, std::string* err) override;
  my_data::TaskResult DoTask(const my_data::Task& task) override;
  std::string Name() const override { return "UUVControl"; }

private:
  std::string device_name_{"uuv-demo"};
  int simulate_latency_ms_{200};
};

} // namespace my_control::demo