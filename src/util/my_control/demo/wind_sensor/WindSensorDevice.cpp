#include "WindSensorDevice.h"

#include <chrono>
#include <random>
#include <thread>

namespace my_control::demo {

bool WindSensorDevice::Init(const nlohmann::json& cfg, std::string* err) {
  try {
    device_name_ = cfg.value("device_name", device_name_);
    simulate_latency_ms_ = cfg.value("simulate_latency_ms", simulate_latency_ms_);
    vx_base_ = cfg.value("vx_base", vx_base_);
    vy_base_ = cfg.value("vy_base", vy_base_);
    vz_base_ = cfg.value("vz_base", vz_base_);
    noise_ = cfg.value("noise", noise_);

    MYLOG_INFO("[Control:{}] Init ok: device_name={}, latency_ms={}, vx_base={}, vy_base={}, vz_base={}, noise={}",
               Name(), device_name_, simulate_latency_ms_, vx_base_, vy_base_, vz_base_, noise_);
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    MYLOG_ERROR("[Control:{}] Init exception: {}", Name(), e.what());
    return false;
  }
}

my_data::TaskResult WindSensorDevice::DoTask(const my_data::Task& task) {
  MYLOG_INFO("[Control:{}] DoTask: device={}, task_id={}, capability={}, action={}, params={}",
             Name(), device_name_, task.task_id, task.capability, task.action, task.params.dump());

  my_data::TaskResult r;
  r.started_at_ms = my_data::NowMs();

  if (task.capability != "wind") {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unsupported capability for split speed sensor: " + task.capability;
    r.finished_at_ms = my_data::NowMs();
    return r;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(simulate_latency_ms_));

  if (task.action == "read") {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<double> nd(0.0, noise_);

    double vx = vx_base_ + nd(rng);
    double vy = vy_base_ + nd(rng);
    double vz = vz_base_ + nd(rng);

    r.code = my_data::ErrorCode::Ok;
    r.message = "split speed read ok";
    r.output = nlohmann::json{
        {"vx", vx},
        {"vy", vy},
        {"vz", vz},
        {"unit", "m/s"},
    };
  } else {
    r.code = my_data::ErrorCode::InvalidCommand;
    r.message = "unknown action for wind: " + task.action;
  }

  r.finished_at_ms = my_data::NowMs();
  MYLOG_INFO("[Control:{}] DoTask done: task_id={}, code={}, message={}, output={}",
             Name(), task.task_id, my_data::ToString(r.code), r.message, r.output.dump());
  return r;
}

} // namespace my_control::demo