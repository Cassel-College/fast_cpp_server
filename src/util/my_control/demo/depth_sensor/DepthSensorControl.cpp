#include "DepthSensorControl.h"

#include <chrono>
#include <random>
#include <thread>

namespace my_control::demo {

bool DepthSensorControl::Init(const nlohmann::json& cfg, std::string* err) {
  try {
    device_name_ = cfg.value("device_name", device_name_);
    simulate_latency_ms_ = cfg.value("simulate_latency_ms", simulate_latency_ms_);
    depth_base_m_ = cfg.value("depth_base_m", depth_base_m_);
    depth_noise_m_ = cfg.value("depth_noise_m", depth_noise_m_);

    MYLOG_INFO("[Control:{}] Init ok: device_name={}, simulate_latency_ms={}, depth_base_m={}, depth_noise_m={}",
               Name(), device_name_, simulate_latency_ms_, depth_base_m_, depth_noise_m_);
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    MYLOG_ERROR("[Control:{}] Init exception: {}", Name(), e.what());
    return false;
  }
}

my_data::TaskResult DepthSensorControl::DoTask(const my_data::Task& task) {
  MYLOG_INFO("[Control:{}] DoTask: device={}, task_id={}, capability={}, action={}, params={}",
             Name(), device_name_, task.task_id, task.capability, task.action, task.params.dump());

  my_data::TaskResult r;
  r.started_at_ms = my_data::NowMs();

  if (task.capability != "water_depth") {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unsupported capability for depth sensor: " + task.capability;
    r.finished_at_ms = my_data::NowMs();
    return r;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(simulate_latency_ms_));

  if (task.action == "read") {
    // demo: base + noise
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<double> nd(0.0, depth_noise_m_);
    double v = depth_base_m_ + nd(rng);

    r.code = my_data::ErrorCode::Ok;
    r.message = "depth read ok";
    r.output = nlohmann::json{
        {"depth_m", v},
        {"unit", "m"},
    };
  } else if (task.action == "calibrate") {
    r.code = my_data::ErrorCode::Ok;
    r.message = "calibrate ok";
    r.output = nlohmann::json{{"calibrated", true}};
  } else {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unknown action for water_depth: " + task.action;
  }

  r.finished_at_ms = my_data::NowMs();
  MYLOG_INFO("[Control:{}] DoTask done: task_id={}, code={}, message={}, output={}",
             Name(), task.task_id, my_data::ToString(r.code), r.message, r.output.dump());
  return r;
}

} // namespace my_control::demo