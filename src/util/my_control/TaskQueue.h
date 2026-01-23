#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>

#include "MyData.h"
#include "MyLog.h"

namespace my_control {

using namespace my_data;
/**
 * @brief 线程安全任务队列（MVP）
 *
 * @details
 * - 生产者：通常是 Edge（接收外部命令后 push task）
 * - 消费者：通常是每个 Device 的 Workflow 线程（阻塞 pop）
 *
 * 语义约定：
 * - PopBlocking() 返回 false：
 *   1) timeout 到期且仍无数据（timeout_ms >= 0 时）
 *   2) 队列已 Shutdown 且队列为空
 * - Shutdown() 会唤醒所有阻塞的 PopBlocking()
 */
class TaskQueue {
public:
  TaskQueue();
  explicit TaskQueue(std::string name);
  ~TaskQueue();

  TaskQueue(const TaskQueue&) = delete;
  TaskQueue& operator=(const TaskQueue&) = delete;

  /**
   * @brief 入队一个 Task（线程安全）
   */
  void Push(const my_data::Task& task);

  /**
   * @brief 阻塞出队
   * @param out 出队的 Task
   * @param timeout_ms 超时时间（毫秒）；<0 表示无限等待
   * @return 是否成功拿到 task
   */
  bool PopBlocking(my_data::Task& out, int timeout_ms = -1);

  /**
   * @brief 队列长度（线程安全）
   */
  std::size_t Size() const;

  /**
   * @brief 清空队列（线程安全）
   */
  void Clear();

  /**
   * @brief 关闭队列并唤醒所有等待线程
   */
  void Shutdown();

  /**
   * @brief 队列是否已关闭
   */
  bool IsShutdown() const;

  /**
   * @brief 队列名字（用于日志与定位）
   */
  const std::string& Name() const { return name_; }

private:
  std::string name_;

  mutable std::mutex mu_;
  std::condition_variable cv_;
  std::deque<my_data::Task> q_;
  bool shutdown_{false};
};

} // namespace my_control