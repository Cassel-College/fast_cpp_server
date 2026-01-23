#include "TaskQueue.h"

#include <chrono>
#include <utility>

namespace my_control {

TaskQueue::TaskQueue() : TaskQueue("TaskQueue") {}

TaskQueue::TaskQueue(std::string name) : name_(std::move(name)) {
  MYLOG_INFO("[TaskQueue:{}] 创建队列实例", name_);
}

TaskQueue::~TaskQueue() {
  // 析构时保证不会卡住等待线程
  Shutdown();
  MYLOG_INFO("[TaskQueue:{}] 析构完成", name_);
}

void TaskQueue::Push(const my_data::Task& task) {
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (shutdown_) {
      MYLOG_WARN("[TaskQueue:{}] Push 被拒绝：队列已 shutdown。task_id={}", name_, task.task_id);
      return;
    }
    q_.push_back(task);
    MYLOG_INFO("[TaskQueue:{}] Push 成功：task_id={}, device_id={}, size={}",
               name_, task.task_id, task.device_id, q_.size());
  }
  cv_.notify_one();
}

bool TaskQueue::PopBlocking(my_data::Task& out, int timeout_ms) {
  std::unique_lock<std::mutex> lk(mu_);

  // 1) 无限等待
  if (timeout_ms < 0) {
    cv_.wait(lk, [&]() { return shutdown_ || !q_.empty(); });
  } else {
    // 2) 超时等待
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    bool ok = cv_.wait_until(lk, deadline, [&]() { return shutdown_ || !q_.empty(); });
    if (!ok) {
      MYLOG_WARN("[TaskQueue:{}] PopBlocking 超时：timeout_ms={}, size={}", name_, timeout_ms, q_.size());
      return false;
    }
  }

  // 被唤醒后：若队列为空
  if (q_.empty()) {
    if (shutdown_) {
      MYLOG_INFO("[TaskQueue:{}] PopBlocking 返回 false：队列已 shutdown 且为空", name_);
    } else {
      // 理论上不会走到这（因为 wait 条件是 !q_.empty()）
      MYLOG_WARN("[TaskQueue:{}] PopBlocking 异常情况：被唤醒但队列为空且未 shutdown", name_);
    }
    return false;
  }

  out = std::move(q_.front());
  q_.pop_front();

  MYLOG_INFO("[TaskQueue:{}] Pop 成功：task_id={}, device_id={}, size={}",
             name_, out.task_id, out.device_id, q_.size());
  return true;
}

std::size_t TaskQueue::Size() const {
  std::lock_guard<std::mutex> lk(mu_);
  return q_.size();
}

void TaskQueue::Clear() {
  std::lock_guard<std::mutex> lk(mu_);
  std::size_t n = q_.size();
  q_.clear();
  MYLOG_WARN("[TaskQueue:{}] Clear：清空 {} 条待执行任务", name_, n);
}

void TaskQueue::Shutdown() {
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (shutdown_) return;
    shutdown_ = true;
    MYLOG_WARN("[TaskQueue:{}] Shutdown：队列关闭，唤醒所有等待线程。size={}", name_, q_.size());
  }
  cv_.notify_all();
}

bool TaskQueue::IsShutdown() const {
  std::lock_guard<std::mutex> lk(mu_);
  return shutdown_;
}

} // namespace my_control