#pragma once

#include <atomic>
#include <nlohmann/json.hpp>
#include <string>

#include "MyData.h"
#include "MyLog.h"
#include "TaskQueue.h" // my_control::TaskQueue

namespace my_device {

using namespace my_control;
/**
 * @brief 设备执行单元接口（每个 device_id 一个实例）
 *
 * @details
 * - Device 内部通常持有：
 *   - 一个 Control（执行器）
 *   - 一个 Workflow（调度线程，消费外部注入的 TaskQueue 引用）
 *   - 一个 DeviceStatus 状态快照（线程安全）
 *
 * 约束（已确认）：
 * - Start 必须传入 TaskQueue&（队列实例归 Edge）
 * - queue_depth 由 Edge 聚合，不在 Device 内维护
 * - 全局 shutdown 时由 Edge 统一 Shutdown 队列
 */
class IDevice {
public:
  virtual ~IDevice() = default;

  /**
   * @brief 初始化设备（加载配置、创建/初始化 control 等）
   */
  virtual bool Init(const nlohmann::json& cfg, std::string* err) = 0;

  /**
   * @brief 启动设备执行单元（注入队列引用 + EStop 引用）
   * @param queue 队列引用（实例归 Edge）
   * @param estop_flag 全局 EStop 标志（可为 nullptr）
   */
  virtual bool Start(my_control::TaskQueue& queue, std::atomic<bool>* estop_flag, std::string* err) = 0;

  /**
   * @brief 请求停止（不负责 shutdown queue）
   */
  virtual void Stop() = 0;

  /**
   * @brief 等待线程退出
   */
  virtual void Join() = 0;

  /**
   * @brief 获取设备状态快照（线程安全）
   */
  virtual my_data::DeviceStatus GetStatusSnapshot() const = 0;

  /**
   * @brief 设备ID
   */
  virtual my_data::DeviceId Id() const = 0;

  /**
   * @brief 设备���型（如 "uuv"）
   */
  virtual std::string Type() const = 0;
};

} // namespace my_device