#include "UUVControl.h"

#include <chrono>
#include <thread>

namespace my_control::demo {

bool UUVControl::Init(const nlohmann::json& cfg, std::string* err) {
  try {
    device_name_ = cfg.value("device_name", device_name_);
    simulate_latency_ms_ = cfg.value("simulate_latency_ms", simulate_latency_ms_);

    MYLOG_INFO("[Control:{}] Init 成功：device_name={}, simulate_latency_ms={}",
               Name(), device_name_, simulate_latency_ms_);
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    MYLOG_ERROR("[Control:{}] Init 异常：{}", Name(), e.what());
    return false;
  }
}

my_data::TaskResult UUVControl::DoTask(const my_data::Task& task) {
  MYLOG_INFO("[Control:{}] DoTask 开始：device={}, task_id={}, capability={}, action={}, params={}",
             Name(), device_name_, task.task_id, task.capability, task.action, task.params.dump());

  my_data::TaskResult r;
  r.started_at_ms = my_data::NowMs();

  // 简单校验
  if (task.capability.empty() || task.action.empty()) {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "capability/action is empty";
    r.finished_at_ms = my_data::NowMs();
    MYLOG_ERROR("[Control:{}] DoTask 失败：{}", Name(), r.message);
    return r;
  }

  // 模拟执行耗时
  std::this_thread::sleep_for(std::chrono::milliseconds(simulate_latency_ms_));

  // demo 动作分支：按 capability + action 组合
  if (task.capability == "navigate") {
    if (task.action == "set") {
      r.code = my_data::ErrorCode::Ok;
      r.message = "navigate set ok";
      r.output = nlohmann::json{{"accepted", true}};
    } else if (task.action == "stop") {
      r.code = my_data::ErrorCode::Ok;
      r.message = "navigate stop ok";
      r.output = nlohmann::json{{"stopped", true}};
    } else {
      r.code = my_data::ErrorCode::InvalidCommand;
      r.message = "unknown action for navigate";
    }
  } else if (task.capability == "sample_water") {
    if (task.action == "start") {
      r.code = my_data::ErrorCode::Ok;
      r.message = "sample start ok";
      r.output = nlohmann::json{{"sample_id", "demo-001"}};
    } else {
      r.code = my_data::ErrorCode::InvalidCommand;
      r.message = "unknown action for sample_water";
    }
  } else if (task.capability == "estop") {
    // 注意：系统级 EStop 通常不走这里。这里只是 demo。
    r.code = my_data::ErrorCode::EStop;
    r.message = "estop requested";
  } else {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unknown capability";
  }

  r.finished_at_ms = my_data::NowMs();

  MYLOG_INFO("[Control:{}] DoTask 完成：task_id={}, code={}, message={}, output={}",
             Name(), task.task_id, my_data::ToString(r.code), r.message, r.output.dump());
  return r;
}

} // namespace my_control::demo