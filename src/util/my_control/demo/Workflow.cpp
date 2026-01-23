#include "Workflow.h"

#include <chrono>
#include <utility>

namespace my_control::demo {

Workflow::Workflow(std::string name, my_control::TaskQueue& queue, my_control::IControl& control)
    : name_(std::move(name)), queue_(queue), control_(control) {
  MYLOG_INFO("[Workflow:{}] 创建：queue={}, control={}", name_, queue_.Name(), control_.Name());
}

Workflow::~Workflow() {
  Stop();
  Join();
  MYLOG_INFO("[Workflow:{}] 析构完成", name_);
}

bool Workflow::Start() {
  if (running_.exchange(true)) {
    MYLOG_WARN("[Workflow:{}] Start 被忽略：已在运行", name_);
    return false;
  }
  stop_ = false;

  MYLOG_INFO("[Workflow:{}] 启动线程", name_);
  worker_ = std::thread(&Workflow::RunLoop, this);
  return true;
}

void Workflow::Stop() {
  if (!running_.load()) return;

  stop_ = true;
  MYLOG_WARN("[Workflow:{}] Stop：请求停止线程", name_);

  // 注意：queue 实例归 Edge，通常由 Edge 在全局 shutdown 时调用 queue.Shutdown()
  // 这里不强制 shutdown queue，以保持“队列归属”边界清晰。
}

void Workflow::Join() {
  if (worker_.joinable()) {
    MYLOG_INFO("[Workflow:{}] Join：等待线程回收...", name_);
    worker_.join();
    MYLOG_INFO("[Workflow:{}] Join：线程已回收", name_);
  }
  running_ = false;
}

void Workflow::RunLoop() {
  MYLOG_INFO("[Workflow:{}] RunLoop 进入", name_);

  while (!stop_.load()) {
    // 1) EStop：不取新任务（MVP）
    if (estop_flag_ && estop_flag_->load()) {
      MYLOG_WARN("[Workflow:{}] EStop=true：暂停取新任务", name_);
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }

    // 2) Pop：使用短超时以便响应 stop_（避免永久阻塞）
    my_data::Task task;
    bool ok = queue_.PopBlocking(task, /*timeout_ms*/ 200);
    if (!ok) {
      // 可能是超时或 shutdown
      if (queue_.IsShutdown()) {
        MYLOG_WARN("[Workflow:{}] 检测到 queue shutdown，退出 RunLoop", name_);
        break;
      }
      continue;
    }

    MYLOG_INFO("[Workflow:{}] 已取到任务：task_id={}, device_id={}, capability={}, action={}",
               name_, task.task_id, task.device_id, task.capability, task.action);

    // 3) on_start：Pop 成功后、DoTask 前触发
    if (on_start_) {
      try {
        MYLOG_INFO("[Workflow:{}] on_start 回调触发：task_id={}", name_, task.task_id);
        on_start_(task);
      } catch (const std::exception& e) {
        MYLOG_ERROR("[Workflow:{}] on_start 回调异常：task_id={}, err={}", name_, task.task_id, e.what());
      } catch (...) {
        MYLOG_ERROR("[Workflow:{}] on_start 回调未知异常：task_id={}", name_, task.task_id);
      }
    }

    // 4) 执行
    MYLOG_INFO("[Workflow:{}] 开始执行 task_id={}", name_, task.task_id);

    my_data::TaskResult result;
    try {
      result = control_.DoTask(task);
    } catch (const std::exception& e) {
      MYLOG_ERROR("[Workflow:{}] DoTask 异常：task_id={}, err={}", name_, task.task_id, e.what());
      result.code = my_data::ErrorCode::InternalError;
      result.message = std::string("DoTask exception: ") + e.what();
    } catch (...) {
      MYLOG_ERROR("[Workflow:{}] DoTask 未知异常：task_id={}", name_, task.task_id);
      result.code = my_data::ErrorCode::InternalError;
      result.message = "DoTask unknown exception";
    }

    MYLOG_INFO("[Workflow:{}] 执行完成 task_id={}, result_code={}, message={}",
               name_, task.task_id, my_data::ToString(result.code), result.message);

    // 5) on_finish：DoTask 后触发
    if (on_finish_) {
      try {
        MYLOG_INFO("[Workflow:{}] on_finish 回调触发：task_id={}", name_, task.task_id);
        on_finish_(task, result);
      } catch (const std::exception& e) {
        MYLOG_ERROR("[Workflow:{}] on_finish 回调异常：{}", name_, e.what());
      } catch (...) {
        MYLOG_ERROR("[Workflow:{}] on_finish 回调未知异常", name_);
      }
    }
  }

  running_ = false;
  MYLOG_WARN("[Workflow:{}] RunLoop 退出", name_);
}

} // namespace my_control::demo