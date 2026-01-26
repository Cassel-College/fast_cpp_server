#include "FlowSensorControl.h"

#include <chrono>
#include <random>
#include <thread>

namespace my_control::demo {

bool FlowSensorControl::Init(const nlohmann::json& cfg, std::string* err) {
  try {
    device_name_ = cfg.value("device_name", device_name_);
    simulate_latency_ms_ = cfg.value("simulate_latency_ms", simulate_latency_ms_);
    flow_base_mps_ = cfg.value("flow_base_mps", flow_base_mps_);
    flow_noise_mps_ = cfg.value("flow_noise_mps", flow_noise_mps_);

    MYLOG_INFO("[Control:{}] Init ok: device_name={}, simulate_latency_ms={}, flow_base_mps={}, flow_noise_mps={}",
               Name(), device_name_, simulate_latency_ms_, flow_base_mps_, flow_noise_mps_);
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    MYLOG_ERROR("[Control:{}] Init exception: {}", Name(), e.what());
    return false;
  }
}

my_data::TaskResult FlowSensorControl::DoTask(const my_data::Task& task) {
  MYLOG_INFO("[Control:{}] DoTask: device={}, task_id={}, capability={}, action={}, params={}",
             Name(), device_name_, task.task_id, task.capability, task.action, task.params.dump());

  my_data::TaskResult r;
  r.started_at_ms = my_data::NowMs();

  if (task.capability != "flow_speed") {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unsupported capability for flow sensor: " + task.capability;
    r.finished_at_ms = my_data::NowMs();
    return r;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(simulate_latency_ms_));

  if (task.action == "read") {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<double> nd(0.0, flow_noise_mps_);
    double v = flow_base_mps_ + nd(rng);

    r.code = my_data::ErrorCode::Ok;
    r.message = "flow read ok";
    r.output = nlohmann::json{
        {"flow_mps", v},
        {"unit", "m/s"},
    };
  } else {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unknown action for flow_speed: " + task.action;
  }

  r.finished_at_ms = my_data::NowMs();
  MYLOG_INFO("[Control:{}] DoTask done: task_id={}, code={}, message={}, output={}",
             Name(), task.task_id, my_data::ToString(r.code), r.message, r.output.dump());
  return r;
}

} // namespace my_control::demo