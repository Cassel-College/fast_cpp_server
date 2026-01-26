#pragma once

#include <memory>
#include <string>

#include "IDevice.h"
#include "MyLog.h"

namespace my_device {

/**
 * @brief my_device 模块门面/工厂
 *
 * @details
 * - 负责按 type 创建设备实例（demo 实现）
 * - 不持有队列；队列由 Edge 创建并注入到 device->Start(queue, ...)
 */
class MyDevice {
public:
  static MyDevice& GetInstance();

  /**
   * @brief 创建具体设备实例
   * @param type 设备类型（如 "uuv"）
   */
  std::unique_ptr<IDevice> Create(const std::string& type);

  /**
   * @brief 创建具体设备实例
   * @param type 设备类型（如 "uuv"）
   * @param cfg 初始化配置
   */
  std::unique_ptr<IDevice> Create(const std::string& type, const nlohmann::json& cfg, std::string* err=nullptr);
  
private:
  MyDevice() = default;
};

} // namespace my_device