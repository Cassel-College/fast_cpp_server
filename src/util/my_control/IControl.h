#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "MyData.h"
#include "MyLog.h"

namespace my_control {

/**
 * @brief 执行器抽象接口：负责真正执行 Task
 *
 * @note
 * - 不负责排队、不负责调度、不负责状态汇总
 * - 由每个 device_id 的 Device 持有一个 IControl 实例（通常是具体设备实现类）
 */
class IControl {
public:
  virtual ~IControl() = default;

  /**
   * @brief 初始化执行器（加载驱动配置）
   */
  virtual bool Init(const nlohmann::json& cfg, std::string* err) = 0;

  /**
   * @brief 执行任务（MVP：同步返回结果）
   */
  virtual my_data::TaskResult DoTask(const my_data::Task& task) = 0;

  /**
   * @brief 健康检查（可选）
   */
  virtual bool HealthCheck() { return true; }

  /**
   * @brief 名称（用于日志）
   */
  virtual std::string Name() const { return "IControl"; }
};

} // namespace my_control