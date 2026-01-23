#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "MyData.h"
#include "MyLog.h"
#include "TaskQueue.h"
#include "IControl.h"

namespace my_control::demo {

/**
 * @brief Workflow（调度线程）：每个 device_id 一条线程
 *
 * @details
 * - 绑定：TaskQueue& + IControl&
 * - 运行：循环 pop task -> (on_start) -> doTask -> (on_finish)
 * - 停止：Stop + Join；通常由 Device/Edge 生命周期控制
 */
class Workflow {
public:
  using StartCallback = std::function<void(const my_data::Task&)>;
  using FinishCallback = std::function<void(const my_data::Task&, const my_data::TaskResult&)>;

  Workflow(std::string name, my_control::TaskQueue& queue, my_control::IControl& control);

  ~Workflow();

  Workflow(const Workflow&) = delete;
  Workflow& operator=(const Workflow&) = delete;

  void SetEStopFlag(std::atomic<bool>* estop_flag) { estop_flag_ = estop_flag; }

  /**
   * @brief 任务开始回调（Pop 成功后、DoTask 前触发）
   */
  void SetStartCallback(StartCallback cb) { on_start_ = std::move(cb); }

  /**
   * @brief 任务完成回调（DoTask 后触发）
   */
  void SetFinishCallback(FinishCallback cb) { on_finish_ = std::move(cb); }

  bool Start();
  void Stop();
  void Join();

  bool IsRunning() const { return running_.load(); }
  const std::string& Name() const { return name_; }

private:
  void RunLoop();

private:
  std::string name_;
  my_control::TaskQueue& queue_;
  my_control::IControl& control_;

  std::atomic<bool> running_{false};
  std::atomic<bool> stop_{false};
  std::thread worker_;

  // 外部注入（由 Edge 或 Device 注入），EStop=true 时不再取新任务
  std::atomic<bool>* estop_flag_{nullptr};

  StartCallback on_start_{};
  FinishCallback on_finish_{};
};

} // namespace my_control::demo