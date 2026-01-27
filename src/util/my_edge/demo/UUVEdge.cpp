#include "UUVEdge.h"

#include <chrono>
#include <thread>
#include <utility>

#include "JsonUtil.h"
#include "MyControl.h"
#include "MyDevice.h"
#include "MyLog.h"
#include "demo/StatusRepository.h"
namespace my_edge::demo {

using namespace my_data;
using namespace my_control;
using namespace my_device;


UUVEdge::UUVEdge() {
  MYLOG_INFO("[Edge:{}] 构造完成 (UUVEdge)", edge_id_);
}

UUVEdge::UUVEdge(const nlohmann::json& cfg, std::string* err) : UUVEdge() {
  this->ShowAnalyzeInitArgs(cfg);
  if (!Init(cfg, err)) {
    MYLOG_ERROR("[Edge:{}] 构造失败 (UUVEdge)：Init 失败", edge_id_);
  } else {
    MYLOG_INFO("[Edge:{}] 构造成功 (UUVEdge)", edge_id_);
  }
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
  // rw_mutex_ must be held with unique_lock by caller (Init path)
  if (normalizers_by_type_.find(type) != normalizers_by_type_.end()) return true;

  MYLOG_INFO("[Edge:{}] 创建 normalizer: type={}", edge_id_, type);

  auto n = my_control::MyControl::GetInstance().CreateNormalizer(type);
  if (!n) {
    std::string e = "CreateNormalizer failed for type=" + type;
    if (err) *err = e;
    MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
    return false;
  }

  normalizers_by_type_[type] = std::move(n);
  MYLOG_INFO("[Edge:{}] normalizer 创建成功: type={} ------------------------------- 完成", edge_id_, type);
  return true;
}

bool UUVEdge::Init(const nlohmann::json& cfg, std::string* err) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  cfg_ = cfg;
  bool initStatus = true;
  edge_id_ = cfg.value("edge_id", edge_id_);
  version_ = cfg.value("version", version_);
  edge_type_ = cfg.value("edge_type", edge_type_);
  allow_queue_when_estop_ = cfg.value("allow_queue_when_estop", false);
  boot_at_ms_ = my_data::NowMs();

  // // snapshot config from cfg.db
  // status_snapshot_enable_ = false;
  // status_snapshot_interval_ms_ = 5000;
  // if (cfg.contains("db") && cfg["db"].is_object()) {
  //   const auto& dbj = cfg["db"];
  //   bool db_enable = dbj.value("enable", false);
  //   bool snap_enable = dbj.value("status_snapshot_enable", false);
  //   status_snapshot_enable_ = db_enable && snap_enable;
  //   status_snapshot_interval_ms_ = dbj.value("status_snapshot_interval_ms", status_snapshot_interval_ms_);
  // }

  // MYLOG_INFO("[Edge:{}] Init 开始：version={}, allow_queue_when_estop={}, snapshot_enable={}, interval_ms={}, cfg={}",
  //            edge_id_, version_, allow_queue_when_estop_, status_snapshot_enable_, status_snapshot_interval_ms_, cfg.dump());

  // clear previous resources (if re-init)
  devices_.clear();
  queues_.clear();
  device_type_by_id_.clear();
  normalizers_by_type_.clear();

  MYLOG_INFO("[Edge:{}] 清理旧资源完成", edge_id_);
  MYLOG_INFO("[Edge:{}] 开始创建设备与队列", edge_id_);

  // devices config
  if (!cfg.contains("devices") || !cfg["devices"].is_array()) {
    // std::string e = "cfg.devices is required and must be array";
    std::string e = "边缘设备需要有末端设备，请补充Device设备信息";
    if (err) *err = e;
    MYLOG_ERROR("[Edge:{}] Init 失败：{}", edge_id_, e);
    run_state_ = RunState::Initializing;
    initStatus = false;
  } else {
    MYLOG_INFO("[Edge:{}] 设备配置数量：{}", edge_id_, cfg["devices"].size());
  }
  int device_index = 0;
  for (const auto& dcfg : cfg["devices"]) {
    MYLOG_INFO("----------------------------------------------------------------------------------------------");
    device_index++;
    MYLOG_INFO("[Edge:{}] 初始化设备 {} / {}: {}", edge_id_, device_index, cfg["devices"].size());
    // dcfg.dump(4));
    // MYLOG_INFO("参数");
    try {
      std::string device_id = dcfg.value("device_id", "");
      std::string type = dcfg.value("type", "");

      if (device_id.empty() || type.empty()) {
        std::string e = "device item missing device_id/type";
        if (err) *err = e;
        MYLOG_ERROR("[Edge:{}] Init 失败：{} item={}", edge_id_, e, dcfg.dump());
        run_state_ = RunState::Initializing;
        initStatus = false;
        continue;
      } else {
        MYLOG_INFO("[Edge:{}] 设备配置：device_id={}, type={}", edge_id_, device_id, type);
      }

      device_type_by_id_[device_id] = type;

      // ensure normalizer for this type
      if (!EnsureNormalizerForTypeLocked(type, err)) {
        run_state_ = RunState::Initializing;
        initStatus = false;
        continue;
      }

      // create queue
      std::string qname = "queue-" + device_id;
      queues_[device_id] = std::make_unique<my_control::TaskQueue>(qname);
      MYLOG_INFO("[Edge:{}] 创建队列：device_id={}, queue={} ------------------------------- 完成", edge_id_, device_id, qname);

      // create device
      auto dev = my_device::MyDevice::GetInstance().Create(type, dcfg, err);
      if (!dev) {
        std::string e = "CreateDevice failed for type=" + type;
        if (err) *err = e;
        MYLOG_ERROR("[Edge:{}] Init 失败：device_id={}, {}", edge_id_, device_id, e);
        run_state_ = RunState::Initializing;
        initStatus = false;
        continue;
      } else {
        MYLOG_INFO("[Edge:{}] Create Device 成功：device_id={}, type={} ------------------------------- 完成", edge_id_, device_id, type);
      }

      devices_[device_id] = std::move(dev);
      MYLOG_INFO("[Edge:{}] 创建设备成功：device_id={}, type={}", edge_id_, device_id, type);
      run_state_ = RunState::Ready;
      MYLOG_INFO("[Edge:{}] Device Init 成功：devices={}, queues={}, run_state={}",
             edge_id_, devices_.size(), queues_.size(), ToString(run_state_.load()));
    } catch (const std::exception& e) {
      std::string emsg = "exception caught: ";
      emsg += e.what();
      if (err) *err = emsg;
      MYLOG_ERROR("[Edge:{}] Init 失败：{}", edge_id_, emsg);
      run_state_ = RunState::Initializing;
      initStatus = false;
      continue;
    }
    MYLOG_INFO("----------------------------------------------------------------------------------------------");
  }
  return initStatus;
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

  // start snapshot thread after running
  StartStatusSnapshotThreadLocked();

  return true;
}

SubmitResult UUVEdge::Submit(const my_data::RawCommand& cmd) {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);

  MYLOG_INFO("[Edge:{}] Submit 开始：command_id={}, source={}, payload={}",
             edge_id_, cmd.command_id, cmd.source, cmd.payload.dump());

  // 1) run_state check (Ready => NotRunning)
  RunState rs = run_state_.load();
  if (rs != RunState::Running) {
    auto r = MakeResult(SubmitCode::NotRunning,
                        "edge is not running, run_state=" + ToString(rs),
                        cmd);
    MYLOG_WARN("[Edge:{}] Submit 拒绝：{}", edge_id_, r.toString());
    return r;
  }

  // 2) estop check (default reject)
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

  // 3) payload must be object and contain device_id (top-level)
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

  // 4) lookup type
  auto dit = device_type_by_id_.find(device_id);
  if (dit == device_type_by_id_.end()) {
    auto r = MakeResult(SubmitCode::UnknownDevice, "unknown device_id=" + device_id, cmd, device_id);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  const std::string& type = dit->second;

  // 5) select normalizer by type
  auto nit = normalizers_by_type_.find(type);
  if (nit == normalizers_by_type_.end() || !nit->second) {
    auto r = MakeResult(SubmitCode::InternalError, "normalizer missing for type=" + type, cmd, device_id);
    MYLOG_ERROR("[Edge:{}] Submit 失败：{}", edge_id_, r.toString());
    return r;
  }

  // 6) normalize
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

  // 7) push to queue
  auto qit = queues_.find(device_id);
  if (qit == queues_.end() || !qit->second) {
    auto r = MakeResult(SubmitCode::InternalError, "queue missing for device_id=" + device_id, cmd, device_id, task.task_id);
    MYLOG_ERROR("[Edge:{}] Submit 失��：{}", edge_id_, r.toString());
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

  std::int64_t pending_total = 0;
  std::int64_t running_total = 0;

  for (const auto& [device_id, dev] : devices_) {
    if (!dev) continue;

    my_data::DeviceStatus ds = dev->GetStatusSnapshot();

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

void UUVEdge::StartStatusSnapshotThreadLocked() {
  // rw_mutex_ must already be held (unique_lock)
  if (!status_snapshot_enable_) {
    MYLOG_INFO("[Edge:{}] StatusSnapshotThread disabled by cfg", edge_id_);
    return;
  }

  // DB not initialized => do not start
  if (!my_db::MyDB::GetInstance().IsInitialized()) {
    MYLOG_WARN("[Edge:{}] StatusSnapshotThread not started: MyDB not initialized", edge_id_);
    return;
  }

  if (snapshot_thread_.joinable()) {
    MYLOG_WARN("[Edge:{}] StatusSnapshotThread already running", edge_id_);
    return;
  }

  snapshot_stop_.store(false);
  MYLOG_INFO("[Edge:{}] StatusSnapshotThread start: interval_ms={}", edge_id_, status_snapshot_interval_ms_);
  snapshot_thread_ = std::thread(&UUVEdge::StatusSnapshotLoop, this);
}

void UUVEdge::StopStatusSnapshotThreadLocked() {
  snapshot_stop_.store(true);
  if (snapshot_thread_.joinable()) {
    MYLOG_INFO("[Edge:{}] StatusSnapshotThread join...", edge_id_);
    snapshot_thread_.join();
    MYLOG_INFO("[Edge:{}] StatusSnapshotThread stopped", edge_id_);
  }
}

void UUVEdge::StatusSnapshotLoop() {
  MYLOG_INFO("[Edge:{}] StatusSnapshotLoop enter", edge_id_);

  while (!snapshot_stop_.load()) {
    my_data::EdgeStatus st = GetStatusSnapshot();

    std::string err;
    auto& repo = my_db::demo::StatusRepository::GetInstance();

    if (!repo.InsertEdgeSnapshot(st, &err)) {
      MYLOG_ERROR("[Edge:{}] StatusSnapshot: InsertEdgeSnapshot failed: {}", edge_id_, err);
    } else {
      MYLOG_DEBUG("[Edge:{}] StatusSnapshot: edge snapshot inserted", edge_id_);
    }

    for (const auto& [device_id, ds] : st.devices) {
      std::string e2;
      if (!repo.InsertDeviceSnapshot(edge_id_, ds, &e2)) {
        MYLOG_ERROR("[Edge:{}] StatusSnapshot: InsertDeviceSnapshot failed: device_id={}, err={}",
                    edge_id_, device_id, e2);
      }
    }

    int interval = status_snapshot_interval_ms_;
    if (interval < 200) interval = 200;
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
  }

  MYLOG_WARN("[Edge:{}] StatusSnapshotLoop exit", edge_id_);
}

void UUVEdge::Shutdown() {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  RunState rs = run_state_.load();
  if (rs == RunState::Stopped || rs == RunState::Stopping) return;

  MYLOG_WARN("[Edge:{}] Shutdown 开始：run_state={}", edge_id_, ToString(rs));
  run_state_ = RunState::Stopping;

  // stop snapshot thread first (avoid concurrent access after clearing maps)
  StopStatusSnapshotThreadLocked();

  // 1) Stop devices (do not shutdown queue here)
  for (auto& [device_id, dev] : devices_) {
    if (!dev) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: device.Stop device_id={}", edge_id_, device_id);
    dev->Stop();
  }

  // 2) Shutdown all queues (global shutdown by Edge)
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


// 新增：解释 Init 入参并记录到日志
void UUVEdge::ShowAnalyzeInitArgs(const nlohmann::json& cfg) const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);
  try {
    MYLOG_INFO("[Edge:{}] ShowAnalyzeInitArgs 开始：cfg={}", edge_id_, cfg.dump(4));
    
    std::string cfg_edge_id = cfg.value("edge_id", std::string("<none>"));
    std::string cfg_version = cfg.value("version", std::string("<none>"));
    bool cfg_allow_queue_when_estop = cfg.value("allow_queue_when_estop", false);
    int devices_count = 0;
    if (cfg.contains("devices") && cfg["devices"].is_array()) {
      devices_count = static_cast<int>(cfg["devices"].size());
    }

    MYLOG_INFO("[Edge:{}] Parsed Init Args: edge_id={}, version={}, allow_queue_when_estop={}, devices_count={}",
               edge_id_, cfg_edge_id, cfg_version, cfg_allow_queue_when_estop, devices_count);

    if (devices_count > 0) {
      for (size_t i = 0; i < cfg["devices"].size(); ++i) {
        const auto& d = cfg["devices"][i];
        std::string did = d.value("device_id", std::string("<none>"));
        std::string dtype = d.value("type", std::string("<none>"));
        std::string dname = d.value("name", std::string("<none>"));
        MYLOG_INFO("[Edge:{}] Device[{}] device_id={}, type={}, name={}", edge_id_, i, did, dtype, dname);
      }
    } else {
      MYLOG_WARN("[Edge:{}] ShowAnalInitArgs: no devices found in cfg", edge_id_);
    }

  } catch (const std::exception& e) {
    MYLOG_ERROR("[Edge:{}] ShowAnalInitArgs 捕获异常: {}", edge_id_, e.what());
  } catch (...) {
    MYLOG_ERROR("[Edge:{}] ShowAnalInitArgs 捕获未知异常", edge_id_);
  }
}


nlohmann::json UUVEdge::DumpInternalInfo() const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);
  nlohmann::json edgeInfo = nlohmann::json::object();
  // 获取类内元素,有几个队列，几个映射关系等，以及设备信息等（仅供调试/日志使用）

  // 获取基本信息
  edgeInfo["edge_id"] = edge_id_;
  edgeInfo["edge_type"] = edge_type_;
  edgeInfo["version"] = version_;
  edgeInfo["run_state"] = ToString(run_state_.load());
  edgeInfo["estop_active"] = estop_.load();

  // 获取任务队列信息
  nlohmann::json queuesJson = nlohmann::json::object();
  for (const auto& [device_id, queue] : queues_) {
    if (!queue) continue;
    nlohmann::json queueInfo;
    queueInfo["name"] = queue->Name();
    queueInfo["size"] = queue->Size();
    queuesJson[device_id] = queueInfo;
  }
  edgeInfo["task-queues"] = queuesJson;

  // 获取设备信息
  nlohmann::json devicesJson = nlohmann::json::object();
  for (const auto& [device_id, dev] : devices_) {
    if (!dev) continue;
    nlohmann::json devInfo;
    // dev->DumpInternalInfo(devInfo);
    devicesJson[device_id] = devInfo;
  }
  edgeInfo["devices"] = devicesJson;

  // 获取 Normalizer 信息
  nlohmann::json normalizersJson = nlohmann::json::object();
  for (const auto& [type, norm] : normalizers_by_type_) {
    if (!norm) continue;
    nlohmann::json normInfo;
    // norm->DumpInternalInfo(normInfo);
    normalizersJson[type] = normInfo;
  }
  edgeInfo["normalizers"] = normalizersJson;
  
  // nlohmann::json devicesJson = nlohmann::json::object();
  // for (const auto& [device_id, dev] : devices_) {
  //   if (!dev) continue;
  //   nlohmann::json devInfo;
  //   dev->DumpInternalInfo(devInfo);
  //   devicesJson[device_id] = devInfo;
  // }
  // edgeInfo["devices"] = devicesJson;

  // nlohmann::json normalizersJson = nlohmann::json::object();
  // for (const auto& [type, norm] : normalizers_by_type_) {
  //   if (!norm) continue;
  //   nlohmann::json normInfo;
  //   norm->DumpInternalInfo(normInfo);
  //   normalizersJson[type] = normInfo;
  // }
  // edgeInfo["normalizers"] = normalizersJson;

  return edgeInfo;
}

} // namespace my_edge::demo