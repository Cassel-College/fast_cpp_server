#include "UUVEdge.h"

#include "JsonUtil.h"
// 依赖 my_device / my_control
#include "MyDevice.h"
#include "MyControl.h"


namespace my_edge::demo {

UUVEdge::UUVEdge() {
  MYLOG_INFO("[Edge:{}] 构造完成 (UUVEdge)", edge_id_);
}

UUVEdge::~UUVEdge() {
  Shutdown();
  MYLOG_INFO("[Edge:{}] 析构完成 (UUVEdge)", edge_id_);
}

std::string UUVEdge::ToString(RunState s) {
  switch (s) {
    case RunState::Initializing: return "Initializing";
    case RunState::Ready: return "Ready";
    case RunState::Running: return "Running";
    case RunState::Stopping: return "Stopping";
    case RunState::Stopped: return "Stopped";
    default: return "UnknownRunState";
  }
}

SubmitResult UUVEdge::MakeResult(SubmitCode code, const std::string& msg,
                                 const my_data::RawCommand& cmd,
                                 const my_data::DeviceId& device_id,
                                 const my_data::TaskId& task_id,
                                 std::int64_t queue_size_after) const {
  SubmitResult r;
  r.code = code;
  r.message = msg;
  r.edge_id = edge_id_;
  r.device_id = device_id;
  r.command_id = cmd.command_id;
  r.task_id = task_id;
  r.queue_size_after = queue_size_after;
  return r;
}

bool UUVEdge::EnsureNormalizerForTypeLocked(const std::string& type, std::string* err) {
  // rw_mutex_ 必须已被 unique_lock 持有（Init 阶段）
  if (normalizers_by_type_.find(type) != normalizers_by_type_.end()) {
    return true;
  }

  MYLOG_INFO("[Edge:{}] 创建 normalizer: type={}", edge_id_, type);

  auto n = my_control::MyControl::GetInstance().CreateNormalizer(type);
  if (!n) {
    std::string e = "CreateNormalizer failed for type=" + type;
    if (err) *err = e;
    MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
    return false;
  }

  normalizers_by_type_[type] = std::move(n);
  MYLOG_INFO("[Edge:{}] normalizer 创建成功: type={}", edge_id_, type);
  return true;
}

bool UUVEdge::Init(const nlohmann::json& cfg, std::string* err) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  cfg_ = cfg;

  edge_id_ = cfg.value("edge_id", edge_id_);
  version_ = cfg.value("version", version_);
  allow_queue_when_estop_ = cfg.value("allow_queue_when_estop", false);
  boot_at_ms_ = my_data::NowMs();

  MYLOG_INFO("[Edge:{}] Init 开始：version={}, allow_queue_when_estop={}, cfg={}",
             edge_id_, version_, allow_queue_when_estop_, cfg.dump());

  // 清理旧资源（若重复 Init）
  devices_.clear();
  queues_.clear();
  device_type_by_id_.clear();
  normalizers_by_type_.clear();

  // devices 配置
  if (!cfg.contains("devices") || !cfg["devices"].is_array()) {
    std::string e = "cfg.devices is required and must be array";
    if (err) *err = e;
    MYLOG_ERROR("[Edge:{}] Init 失败：{}", edge_id_, e);
    run_state_ = RunState::Initializing;
    return false;
  }

  // 逐设备创建 queue + device
  for (const auto& dcfg : cfg["devices"]) {
    std::string device_id = dcfg.value("device_id", "");
    std::string type = dcfg.value("type", "");

    if (device_id.empty() || type.empty()) {
      std::string e = "device item missing device_id/type";
      if (err) *err = e;
      MYLOG_ERROR("[Edge:{}] Init 失败：{} item={}", edge_id_, e, dcfg.dump());
      run_state_ = RunState::Initializing;
      return false;
    }

    // 记录映射
    device_type_by_id_[device_id] = type;

    // 准备 normalizer（按 type）
    if (!EnsureNormalizerForTypeLocked(type, err)) {
      run_state_ = RunState::Initializing;
      return false;
    }

    // 创建队列
    std::string qname = "queue-" + device_id;
    auto q = std::make_unique<my_control::TaskQueue>(qname);
    MYLOG_INFO("[Edge:{}] 创建队列：device_id={}, queue={}", edge_id_, device_id, qname);
    queues_[device_id] = std::move(q);

    // 创建设备
    auto dev = my_device::MyDevice::GetInstance().Create(type);
    if (!dev) {
      std::string e = "CreateDevice failed for type=" + type;
      if (err) *err = e;
      MYLOG_ERROR("[Edge:{}] Init 失败：device_id={}, {}", edge_id_, device_id, e);
      run_state_ = RunState::Initializing;
      return false;
    }

    // 初始化设备（但不 Start）
    std::string dev_err;
    if (!dev->Init(dcfg, &dev_err)) {
      std::string e = "device.Init failed: " + dev_err;
      if (err) *err = e;
      MYLOG_ERROR("[Edge:{}] Init 失败：device_id={}, {}", edge_id_, device_id, e);
      run_state_ = RunState::Initializing;
      return false;
    }

    MYLOG_INFO("[Edge:{}] 创建设备成功：device_id={}, type={}", edge_id_, device_id, type);
    devices_[device_id] = std::move(dev);
  }

  run_state_ = RunState::Ready;
  MYLOG_INFO("[Edge:{}] Init 成功：devices={}, queues={}, run_state={}",
             edge_id_, devices_.size(), queues_.size(), ToString(run_state_.load()));
  return true;
}

bool UUVEdge::Start(std::string* err) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  if (run_state_.load() != RunState::Ready) {
    std::string e = "Start rejected: run_state=" + ToString(run_state_.load());
    if (err) *err = e;
    MYLOG_WARN("[Edge:{}] {}", edge_id_, e);
    return false;
  }

  MYLOG_INFO("[Edge:{}] Start 开始：devices={}", edge_id_, devices_.size());

  // 启动每个 device（注入 queue + estop）
  for (auto& [device_id, dev] : devices_) {
    auto qit = queues_.find(device_id);
    if (qit == queues_.end()) {
      std::string e = "queue missing for device_id=" + device_id;
      if (err) *err = e;
      MYLOG_ERROR("[Edge:{}] Start 失败：{}", edge_id_, e);
      run_state_ = RunState::Ready;
      return false;
    }

    std::string dev_err;
    if (!dev->Start(*qit->second, &estop_, &dev_err)) {
      std::string e = "device.Start failed: device_id=" + device_id + ", err=" + dev_err;
      if (err) *err = e;
      MYLOG_ERROR("[Edge:{}] Start 失败：{}", edge_id_, e);
      run_state_ = RunState::Ready;
      return false;
    }

    MYLOG_INFO("[Edge:{}] device.Start 成功：device_id={}, queue={}",
               edge_id_, device_id, qit->second->Name());
  }

  run_state_ = RunState::Running;
  MYLOG_INFO("[Edge:{}] Start 成功：run_state={}", edge_id_, ToString(run_state_.load()));
  return true;
}

SubmitResult UUVEdge::Submit(const my_data::RawCommand& cmd) {
  // Submit 为高频：shared_lock
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);

  MYLOG_INFO("[Edge:{}] Submit 开始：command_id={}, source={}, payload={}",
             edge_id_, cmd.command_id, cmd.source, cmd.payload.dump());

  // 1) 状态校验：Ready 状态拒绝（你已确认）
  RunState rs = run_state_.load();
  if (rs != RunState::Running) {
    auto r = MakeResult(SubmitCode::NotRunning,
                        "edge is not running, run_state=" + ToString(rs),
                        cmd);
    MYLOG_WARN("[Edge:{}] Submit 拒绝：{}", edge_id_, r.toString());
    return r;
  }

  // 2) EStop 校验：默认拒绝，可配置允许入队
  if (estop_.load() && !allow_queue_when_estop_) {
    std::string reason;
    {
      std::lock_guard<std::mutex> lk2(estop_mu_);
      reason = estop_reason_;
    }
    auto r = MakeResult(SubmitCode::EStop,
                        reason.empty() ? "estop active" : ("estop active: " + reason),
                        cmd);
    MYLOG_WARN("[Edge:{}] Submit 拒绝(EStop)：{}", edge_id_, r.toString());
    return r;
  }

  // 3) 从 payload 顶层读取 device_id（强制要求）
  if (!cmd.payload.is_object()) {
    auto r = MakeResult(SubmitCode::InvalidCommand, "payload must be object", cmd);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  std::string device_id = my_data::jsonutil::GetStringOr(cmd.payload, "device_id", "");
  if (device_id.empty()) {
    auto r = MakeResult(SubmitCode::InvalidCommand, "missing payload.device_id", cmd);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  // 4) 查 device_id 是否存在
  auto dit = device_type_by_id_.find(device_id);
  if (dit == device_type_by_id_.end()) {
    auto r = MakeResult(SubmitCode::UnknownDevice, "unknown device_id=" + device_id, cmd, device_id);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  const std::string& type = dit->second;

  // 5) 选 normalizer
  auto nit = normalizers_by_type_.find(type);
  if (nit == normalizers_by_type_.end() || !nit->second) {
    auto r = MakeResult(SubmitCode::InternalError, "normalizer missing for type=" + type, cmd, device_id);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  // 6) Normalize
  std::string nerr;
  auto maybe_task = nit->second->Normalize(cmd, edge_id_, &nerr);
  if (!maybe_task.has_value()) {
    auto r = MakeResult(SubmitCode::InvalidCommand,
                        nerr.empty() ? "normalize failed" : ("normalize failed: " + nerr),
                        cmd, device_id);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  my_data::Task task = *maybe_task;

  // 7) 找队列并入队
  auto qit = queues_.find(device_id);
  if (qit == queues_.end() || !qit->second) {
    auto r = MakeResult(SubmitCode::InternalError, "queue missing for device_id=" + device_id, cmd, device_id, task.task_id);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  if (qit->second->IsShutdown()) {
    auto r = MakeResult(SubmitCode::QueueShutdown, "queue already shutdown", cmd, device_id, task.task_id);
    MYLOG_WARN("[Edge:{}] Submit 拒绝：{}", edge_id_, r.toString());
    return r;
  }

  qit->second->Push(task);
  std::int64_t qsize = static_cast<std::int64_t>(qit->second->Size());

  auto r = MakeResult(SubmitCode::Ok, "queued", cmd, device_id, task.task_id, qsize);
  MYLOG_INFO("[Edge:{}] Submit 成功：{}", edge_id_, r.toString());
  return r;
}

my_data::EdgeStatus UUVEdge::GetStatusSnapshot() const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);

  my_data::EdgeStatus s;
  s.edge_id = edge_id_;
  s.boot_at_ms = boot_at_ms_;
  s.version = version_;

  // run_state -> my_data::EdgeRunState
  RunState rs = run_state_.load();
  if (rs == RunState::Initializing || rs == RunState::Ready) {
    s.run_state = my_data::EdgeRunState::Initializing;
  } else if (rs == RunState::Running) {
    s.run_state = estop_.load() ? my_data::EdgeRunState::EStop : my_data::EdgeRunState::Running;
  } else if (rs == RunState::Stopping) {
    s.run_state = my_data::EdgeRunState::Degraded;
  } else {
    s.run_state = my_data::EdgeRunState::Degraded;
  }

  s.estop_active = estop_.load();
  {
    std::lock_guard<std::mutex> lk2(estop_mu_);
    s.estop_reason = estop_reason_;
  }

  // 汇总 devices + queue_depth
  std::int64_t pending_total = 0;
  std::int64_t running_total = 0;

  for (const auto& [device_id, dev] : devices_) {
    if (!dev) continue;

    my_data::DeviceStatus ds = dev->GetStatusSnapshot();

    // queue_depth 由 Edge 聚合（你已确认）
    auto qit = queues_.find(device_id);
    if (qit != queues_.end() && qit->second) {
      ds.queue_depth = static_cast<std::int64_t>(qit->second->Size());
      pending_total += ds.queue_depth;
    }

    if (ds.work_state == my_data::DeviceWorkState::Busy) {
      running_total += 1;
    }

    s.devices[device_id] = ds;
  }

  s.tasks_pending_total = pending_total;
  s.tasks_running_total = running_total;

  MYLOG_DEBUG("[Edge:{}] GetStatusSnapshot：devices={}, pending_total={}, running_total={}, estop={}",
              edge_id_, s.devices.size(), s.tasks_pending_total, s.tasks_running_total, s.estop_active);
  return s;
}

void UUVEdge::SetEStop(bool active, const std::string& reason) {
  {
    std::unique_lock<std::shared_mutex> lk(rw_mutex_);
    estop_.store(active);
    {
      std::lock_guard<std::mutex> lk2(estop_mu_);
      estop_reason_ = reason;
    }
  }
  MYLOG_WARN("[Edge:{}] SetEStop：active={}, reason={}", edge_id_, active ? "true" : "false", reason);
}

void UUVEdge::Shutdown() {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  RunState rs = run_state_.load();
  if (rs == RunState::Stopped || rs == RunState::Stopping) {
    return;
  }

  MYLOG_WARN("[Edge:{}] Shutdown 开始：run_state={}", edge_id_, ToString(rs));
  run_state_ = RunState::Stopping;

  // 1) Stop devices（不 shutdown queue）
  for (auto& [device_id, dev] : devices_) {
    if (!dev) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: device.Stop device_id={}", edge_id_, device_id);
    dev->Stop();
  }

  // 2) Shutdown all queues（你已确认：全局 shutdown 由 Edge 统一 shutdown 队列）
  for (auto& [device_id, q] : queues_) {
    if (!q) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: queue.Shutdown device_id={}, queue={}", edge_id_, device_id, q->Name());
    q->Shutdown();
  }

  // 3) Join devices
  for (auto& [device_id, dev] : devices_) {
    if (!dev) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: device.Join device_id={}", edge_id_, device_id);
    dev->Join();
  }

  devices_.clear();
  queues_.clear();
  device_type_by_id_.clear();
  normalizers_by_type_.clear();

  run_state_ = RunState::Stopped;
  MYLOG_WARN("[Edge:{}] Shutdown 完成：run_state={}", edge_id_, ToString(run_state_.load()));
}

} // namespace my_edge::demo