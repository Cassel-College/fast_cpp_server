#include "UUVDevice.h"

#include <utility>

namespace my_device::demo {

UUVDevice::UUVDevice() {
  {
    std::lock_guard<std::mutex> lk(status_mu_);
    status_.device_id = device_id_;
    status_.conn_state = my_data::DeviceConnState::Unknown;
    status_.work_state = my_data::DeviceWorkState::Idle;
    status_.running_task_id = "";
    status_.last_error = "";
    status_.last_seen_at_ms = my_data::NowMs();
  }
  MYLOG_INFO("[Device:{}] 构造完成 (UUVDevice)", device_id_);
}

UUVDevice::UUVDevice(const nlohmann::json& cfg, std::string* err) : UUVDevice() {
  MYLOG_INFO("[Device:{}] 通过配置构造 UUVDevice，cfg={}", device_id_, cfg.dump());
  try {
    if (!Init(cfg, err)) {
      MYLOG_ERROR("[Device:{}] 通过配置构造失败，err={}", device_id_, err ? *err : "unknown");
    } else {
      MYLOG_INFO("[Device:{}] 通过配置构造成功", device_id_);
    }
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    MYLOG_ERROR("[Device:{}] 通过配置构造异常：{}", device_id_, e.what());
  }
}

UUVDevice::~UUVDevice() {
  Stop();
  Join();
  MYLOG_INFO("[Device:{}] 析构完成 (UUVDevice)", device_id_);
}

bool UUVDevice::Init(const nlohmann::json& cfg, std::string* err) {
  MYLOG_INFO("[Device:{}] Init 开始，cfg={}", device_id_, cfg.dump());

  try {
    // 1) 解析设备基础信息
    device_id_ = cfg.value("device_id", device_id_);
    device_name_ = cfg.value("device_name", device_name_);

    {
      std::lock_guard<std::mutex> lk(status_mu_);
      status_.device_id = device_id_;
      status_.conn_state = my_data::DeviceConnState::Online;
      status_.work_state = my_data::DeviceWorkState::Idle;
      status_.last_seen_at_ms = my_data::NowMs();
    }

    // 2) 创建并初始化 control（通过 my_control 的工厂）
    MYLOG_INFO("[Device:{}] 创建 Control 实例...", device_id_);
    control_ = my_control::MyControl::GetInstance().CreateControl("uuv");
    if (!control_) {
      if (err) *err = "CreateControl(uuv) failed";
      MYLOG_ERROR("[Device:{}] Init 失败：CreateControl(uuv) 返回 nullptr", device_id_);
      return false;
    }

    nlohmann::json control_cfg = cfg.value("control", nlohmann::json::object());
    std::string control_err;
    if (!control_->Init(control_cfg, &control_err)) {
      if (err) *err = control_err;
      MYLOG_ERROR("[Device:{}] Init 失败：control.Init 失败，err={}", device_id_, control_err);
      return false;
    }

    MYLOG_INFO("[Device:{}] Init 成功：device_name={}, control={}",
               device_id_, device_name_, control_->Name());
    return true;

  } catch (const std::exception& e) {
    if (err) *err = e.what();
    MYLOG_ERROR("[Device:{}] Init 异常：{}", device_id_, e.what());
    return false;
  }
}

bool UUVDevice::Start(my_control::TaskQueue& queue, std::atomic<bool>* estop_flag, std::string* err) {
  MYLOG_INFO("[Device:{}] Start 开始：注入 queue={}, estop_ptr={}",
             device_id_, queue.Name(), (void*)estop_flag);

  if (!control_) {
    if (err) *err = "device not initialized: control_ is null";
    MYLOG_ERROR("[Device:{}] Start 失败：control_ 为空（请先 Init）", device_id_);
    return false;
  }

  queue_ = &queue;
  estop_flag_ = estop_flag;

  // 创建 workflow（每设备一条线程）
  std::string wf_name = "wf-" + device_id_;
  workflow_ = std::make_unique<my_control::demo::Workflow>(wf_name, queue, *control_);

  // 注入 estop
  workflow_->SetEStopFlag(estop_flag_);

  // 任务开始回调：准确设置 Busy + running_task_id
  workflow_->SetStartCallback([this](const my_data::Task& task) {
    this->UpdateOnTaskStart(task);
  });

  // 任务完成回调：清理 running_task + 记录 last_result
  workflow_->SetFinishCallback([this](const my_data::Task& task, const my_data::TaskResult& r) {
    this->UpdateOnTaskFinish(task, r);
  });

  bool started = workflow_->Start();
  if (!started) {
    if (err) *err = "workflow already running or start failed";
    MYLOG_ERROR("[Device:{}] Start 失败：Workflow 未能启动", device_id_);
    return false;
  }

  MYLOG_INFO("[Device:{}] Start 成功：Workflow 已启动，wf_name={}", device_id_, wf_name);
  return true;
}

void UUVDevice::Stop() {
  MYLOG_WARN("[Device:{}] Stop：请求停止 Device", device_id_);

  // 不负责 shutdown queue（归 Edge）；仅请求 workflow 停止
  if (workflow_) {
    workflow_->Stop();
  }

  {
    std::lock_guard<std::mutex> lk(status_mu_);
    status_.last_seen_at_ms = my_data::NowMs();
  }
}

void UUVDevice::Join() {
  if (workflow_) {
    MYLOG_INFO("[Device:{}] Join：等待 Workflow 退出...", device_id_);
    workflow_->Join();
    MYLOG_INFO("[Device:{}] Join：Workflow 已退出", device_id_);
  }
}

my_data::DeviceStatus UUVDevice::GetStatusSnapshot() const {
  std::lock_guard<std::mutex> lk(status_mu_);
  return status_;
}

void UUVDevice::UpdateOnTaskStart(const my_data::Task& task) {
  std::lock_guard<std::mutex> lk(status_mu_);
  status_.work_state = my_data::DeviceWorkState::Busy;
  status_.running_task_id = task.task_id;
  status_.last_seen_at_ms = my_data::NowMs();
  running_task_snapshot_ = task;

  MYLOG_INFO("[Device:{}] 状态更新：进入 Busy，running_task_id={}, capability={}, action={}",
             device_id_, status_.running_task_id, task.capability, task.action);
}

void UUVDevice::UpdateOnTaskFinish(const my_data::Task& task, const my_data::TaskResult& r) {
  std::lock_guard<std::mutex> lk(status_mu_);

  status_.last_task_at_ms = my_data::NowMs();
  status_.last_seen_at_ms = status_.last_task_at_ms;

  if (r.code == my_data::ErrorCode::Ok) {
    status_.last_error = "";
    status_.work_state = my_data::DeviceWorkState::Idle;
  } else {
    status_.last_error = r.message.empty() ? my_data::ToString(r.code) : r.message;
    status_.work_state = my_data::DeviceWorkState::Faulted;
  }

  status_.running_task_id.clear();
  running_task_snapshot_.reset();

  MYLOG_INFO("[Device:{}] 状态更新：任务完成 task_id={}, code={}, work_state={}, last_error={}",
             device_id_,
             task.task_id,
             my_data::ToString(r.code),
             my_data::ToString(status_.work_state),
             status_.last_error);
}

} // namespace my_device::demo