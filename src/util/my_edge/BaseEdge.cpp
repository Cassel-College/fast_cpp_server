#include "BaseEdge.h"

#include <chrono>
#include <atomic>

#include "JsonUtil.h"
#include "MyDevice.h"
#include "MyLog.h"

namespace my_edge {

using namespace my_data;
using namespace my_device;

BaseEdge::BaseEdge() {
  MYLOG_INFO("[BaseEdge] 构造完成，默认 edge_type={}", edge_type_);
}

BaseEdge::BaseEdge(std::string edge_type) : edge_type_(std::move(edge_type)) {
  MYLOG_INFO("[BaseEdge] 构造完成，edge_type={}", edge_type_);
}

BaseEdge::~BaseEdge() {
  Shutdown();
  MYLOG_INFO("[Edge:{}] BaseEdge 析构完成", edge_id_);
}

bool BaseEdge::Init(const nlohmann::json& cfg, std::string* err) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);
  run_state_ = RunState::Initializing;

  cfg_ = cfg;
  edge_id_ = cfg.value("edge_id", edge_id_);
  version_ = cfg.value("version", version_);
  edge_type_ = cfg.value("edge_type", edge_type_);
  allow_queue_when_estop_ = cfg.value("allow_queue_when_estop", false);

  // self/snapshot 配置
  self_action_enable_ = cfg.value("self_action_enable", self_action_enable_);
  self_device_id_ = cfg.value("self_device_id", self_device_id_);
  snapshot_enable_ = cfg.value("snapshot_enable", snapshot_enable_);
  snapshot_interval_ms_ = cfg.value("snapshot_interval_ms", snapshot_interval_ms_);

  boot_at_ms_ = my_data::NowMs();

  // 清理旧资源（支持重复 Init）
  devices_.clear();
  queues_.clear();
  device_type_by_id_.clear();

  MYLOG_INFO("[Edge:{}] Init 开始: edge_type={}, version={}, self_action_enable={}, snapshot_enable={}, interval_ms={}",
             edge_id_, edge_type_, self_action_enable_ ? "true" : "false",
             snapshot_enable_ ? "true" : "false", snapshot_interval_ms_);

  // 1) 永远创建 self 队列（Edge ��身能力）
  {
    const std::string qname = "queue-" + self_device_id_;
    queues_[self_device_id_] = std::make_unique<my_control::TaskQueue>(qname);
    device_type_by_id_[self_device_id_] = "self";
    MYLOG_INFO("[Edge:{}] 创建 self 队列成功: device_id={}, queue={}", edge_id_, self_device_id_, qname);
  }

  // 2) devices 是可选的
  if (cfg.contains("devices") && cfg["devices"].is_array()) {
    int idx = 0;
    for (const auto& dcfg : cfg["devices"]) {
      idx++;
      try {
        std::string device_id = dcfg.value("device_id", "");
        std::string type = dcfg.value("type", "");
        if (device_id.empty() || type.empty()) {
          std::string e = "devices[" + std::to_string(idx) + "] 缺少 device_id/type";
          MYLOG_ERROR("[Edge:{}] Init 失败: {}，item={}", edge_id_, e, dcfg.dump());
          if (err) *err = e;
          continue; // 不直接 return false，让 Edge 仍能以 self 模式运行
        }

        device_type_by_id_[device_id] = type;

        std::string qname = "queue-" + device_id;
        queues_[device_id] = std::make_unique<my_control::TaskQueue>(qname);
        MYLOG_INFO("[Edge:{}] 创建队列: device_id={}, type={}, queue={}", edge_id_, device_id, type, qname);

        auto dev = my_device::MyDevice::GetInstance().Create(type, dcfg, err);
        if (!dev) {
          std::string e = "创建设备失败: device_id=" + device_id + ", type=" + type;
          MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
          if (err) *err = e;
          continue;
        }
        devices_[device_id] = std::move(dev);
        MYLOG_INFO("[Edge:{}] 创建设备成功: device_id={}, type={}", edge_id_, device_id, type);
      } catch (const std::exception& e) {
        std::string emsg = std::string("Init 捕获异常: ") + e.what();
        MYLOG_ERROR("[Edge:{}] {}", edge_id_, emsg);
        if (err) *err = emsg;
        continue;
      }
    }
  } else {
    MYLOG_WARN("[Edge:{}] 未配置 devices，Edge 将以 self 模式运行（仅 self_action + snapshot）", edge_id_);
  }

  MYLOG_INFO("注册内置 say_hello handler");
  RegisterSelfTaskHandler("say_hello", [this](const my_data::Task& task) {this->SayHelloAction(task); });

  run_state_ = RunState::Ready;
  MYLOG_INFO("[Edge:{}] Init 完成: run_state=Ready，devices={}, queues={}",
             edge_id_, devices_.size(), queues_.size());
  return true;
}

bool BaseEdge::Start(std::string* err) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  if (run_state_.load() != RunState::Ready) {
    std::string e = "Start 被拒绝: 当前 run_state=" + RunStateToString(run_state_.load());
    MYLOG_WARN("[Edge:{}] {}", edge_id_, e);
    if (err) *err = e;
    return false;
  }

  MYLOG_INFO("[Edge:{}] Start 开始: devices={}", edge_id_, devices_.size());

  // 1) 启动 self task 监控线程
  StartSelfTaskMonitorThreadLocked();
  
  // 2) 启动 self action thread
  StartSelfActionThreadLocked();

  // 3) 启动心跳/上报线程
  StartSnapshotThreadLocked();

  // 4) 启动所有 device（如果有）
  for (auto& [device_id, dev] : devices_) {
    auto qit = queues_.find(device_id);
    if (qit == queues_.end() || !qit->second) {
      std::string e = "Start 失败: 找不到 device_id=" + device_id + " 对应的队列";
      MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
      if (err) *err = e;
      return false;
    }

    std::string dev_err;
    if (!dev->Start(*qit->second, &estop_, &dev_err)) {
      std::string e = "device.Start 失败: device_id=" + device_id + ", err=" + dev_err;
      MYLOG_ERROR("[Edge:{}] {}", edge_id_, e);
      if (err) *err = e;
      return false;
    }

    MYLOG_INFO("[Edge:{}] device.Start 成功: device_id={}, queue={}", edge_id_, device_id, qit->second->Name());
  }

  run_state_ = RunState::Running;
  MYLOG_INFO("[Edge:{}] Start 成功: run_state=Running", edge_id_);
  return true;
}

SubmitResult BaseEdge::Submit(const my_data::RawCommand& cmd) {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);

  MYLOG_INFO("[Edge:{}] Submit: command_id={}, source={}, payload={}",
             edge_id_, cmd.command_id, cmd.source, cmd.payload.dump());

  // 1) run_state
  if (run_state_.load() != RunState::Running) {
    return MakeResult(SubmitCode::NotRunning,
                      "edge 未运行，run_state=" + RunStateToString(run_state_.load()),
                      cmd);
  }

  // 2) estop
  if (estop_.load() && !allow_queue_when_estop_) {
    std::string reason;
    {
      std::lock_guard<std::mutex> lk2(estop_mu_);
      reason = estop_reason_;
    }
    return MakeResult(SubmitCode::EStop,
                      reason.empty() ? "estop 已激活" : ("estop 已激活: " + reason),
                      cmd);
  }

  // 3) payload 校验
  if (!cmd.payload.is_object()) {
    return MakeResult(SubmitCode::InvalidCommand, "payload 必须是对象", cmd);
  }

  // 4) device_id
  std::string device_id = my_data::jsonutil::GetStringOr(cmd.payload, "device_id", "");
  if (device_id.empty()) {
    return MakeResult(SubmitCode::InvalidCommand, "缺少 payload.device_id", cmd);
  }

  // 5) type
  auto dit = device_type_by_id_.find(device_id);
  if (dit == device_type_by_id_.end()) {
    return MakeResult(SubmitCode::UnknownDevice, "未知 device_id=" + device_id, cmd, device_id);
  }
  const std::string& type = dit->second;

  // 6) normalize（由子类决定策略）
  std::string nerr;
  auto maybe_task = NormalizeCommandLocked(cmd, device_id, type, &nerr);
  if (!maybe_task.has_value()) {
    return MakeResult(SubmitCode::InvalidCommand,
                      nerr.empty() ? "Normalize 失败" : ("Normalize 失败: " + nerr),
                      cmd, device_id);
  }
  my_data::Task task = *maybe_task;

  // 7) push queue
  std::string qerr;
  if (!AppendTaskToQueueLocked(device_id, task, &qerr)) {
    return MakeResult(SubmitCode::InternalError,
                      qerr.empty() ? "入队失败" : ("入队失败: " + qerr),
                      cmd, device_id, task.task_id);
  }

  auto qit = queues_.find(device_id);
  std::int64_t qsize = (qit != queues_.end() && qit->second)
                         ? static_cast<std::int64_t>(qit->second->Size())
                         : 0;

  return MakeResult(SubmitCode::Ok, "已入队", cmd, device_id, task.task_id, qsize);
}

my_data::EdgeStatus BaseEdge::GetStatusSnapshot() const {
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

  // 也把 self 的队列深度统计进去（self 没有 DeviceStatus，可放到 tasks_pending_total）
  auto self_qit = queues_.find(self_device_id_);
  if (self_qit != queues_.end() && self_qit->second) {
    pending_total += static_cast<std::int64_t>(self_qit->second->Size());
  }

  s.tasks_pending_total = pending_total;
  s.tasks_running_total = running_total;
  return s;
}

void BaseEdge::SetEStop(bool active, const std::string& reason) {
  {
    std::unique_lock<std::shared_mutex> lk(rw_mutex_);
    estop_.store(active);
    {
      std::lock_guard<std::mutex> lk2(estop_mu_);
      estop_reason_ = reason;
    }
  }
  MYLOG_WARN("[Edge:{}] SetEStop: active={}, reason={}",
             edge_id_, active ? "true" : "false", reason);
}

void BaseEdge::Shutdown() {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  RunState rs = run_state_.load();
  if (rs == RunState::Stopped || rs == RunState::Stopping) return;

  MYLOG_WARN("[Edge:{}] Shutdown 开始: run_state={}", edge_id_, RunStateToString(rs));
  run_state_ = RunState::Stopping;

  // 先停线程（避免线程访问被清理的 queues_/devices_）
  StopSnapshotThreadLocked();
  StopSelfActionThreadLocked();

  // stop devices
  for (auto& [device_id, dev] : devices_) {
    if (!dev) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: device.Stop device_id={}", edge_id_, device_id);
    dev->Stop();
  }

  // shutdown queues（包含 self）
  for (auto& [device_id, q] : queues_) {
    if (!q) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: queue.Shutdown device_id={}, queue={}", edge_id_, device_id, q->Name());
    q->Shutdown();
  }

  // join devices
  for (auto& [device_id, dev] : devices_) {
    if (!dev) continue;
    MYLOG_WARN("[Edge:{}] Shutdown: device.Join device_id={}", edge_id_, device_id);
    dev->Join();
  }

  devices_.clear();
  queues_.clear();
  device_type_by_id_.clear();

  run_state_ = RunState::Stopped;
  MYLOG_WARN("[Edge:{}] Shutdown 完成: run_state=Stopped", edge_id_);
}

my_data::EdgeId BaseEdge::Id() const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);
  return edge_id_;
}

std::string BaseEdge::EdgeType() const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);
  return edge_type_;
}

void BaseEdge::ShowAnalyzeInitArgs(const nlohmann::json& cfg) const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);
  MYLOG_INFO("[Edge:{}] ShowAnalyzeInitArgs: cfg={}", edge_id_, cfg.dump(4));
}

nlohmann::json BaseEdge::DumpInternalInfo() const {
  std::shared_lock<std::shared_mutex> lk(rw_mutex_);

  nlohmann::json j;
  j["edge_id"] = edge_id_;
  j["edge_type"] = edge_type_;
  j["version"] = version_;
  j["run_state"] = RunStateToString(run_state_.load());
  j["estop_active"] = estop_.load();
  j["self_device_id"] = self_device_id_;
  j["self_action_enable"] = self_action_enable_;
  j["snapshot_enable"] = snapshot_enable_;
  j["snapshot_interval_ms"] = snapshot_interval_ms_;

  nlohmann::json qj = nlohmann::json::object();
  for (const auto& [device_id, q] : queues_) {
    if (!q) continue;
    qj[device_id] = {{"name", q->Name()}, {"size", q->Size()}};
  }
  j["queues"] = qj;

  nlohmann::json dj = nlohmann::json::array();
  for (const auto& [device_id, _] : devices_) {
    dj.push_back(device_id);
  }
  j["devices"] = dj;

  return j;
}

nlohmann::json BaseEdge::GetRunTimeStatusInfo() const {
  MYLOG_INFO("[Edge:{}] 开始获取运行时状态信息", edge_id_);
  nlohmann::json allRunningInfo;
  nlohmann::json tj = nlohmann::json::object();


  // 线程状态
  tj["self_task_monitor"] = {
    {"enabled", self_task_monitor_enable_},
    {"running", self_task_monitor_thread_.joinable()},
    {"boot_at_ms", self_task_monitor_boot_at_ms_},
    {"running_time_s", int((my_data::NowMs() - self_task_monitor_boot_at_ms_) / 1000)}
  };
  tj["self_action"] = {
    {"enabled", self_action_enable_},
    {"running", self_action_thread_.joinable()},
    {"boot_at_ms", self_action_boot_at_ms_},
    {"running_time_s", int((my_data::NowMs() - self_action_boot_at_ms_) / 1000)}
  };
  tj["snapshot"] = {
    {"enabled", snapshot_enable_},
    {"running", snapshot_thread_.joinable()},
    {"boot_at_ms", snapshot_boot_at_ms_},
    {"running_time_s", int((my_data::NowMs() - snapshot_boot_at_ms_) / 1000)}
  };
  allRunningInfo["thread_status"] = tj;

  // 任务队列状态
  nlohmann::json qj = nlohmann::json::object();
  for (const auto& [device_id, q] : queues_) {
    if (!q) continue;
    qj[device_id] = {
      {"name", q->Name()},
      {"size", q->Size()},
      {"is_shutdown", q->IsShutdown()}
    };
  }
  allRunningInfo["queues"] = qj;
  return allRunningInfo;
}


bool BaseEdge::AppendJsonTask(const nlohmann::json& taskj) {
  try {
    my_data::Task t = my_data::Task::fromJson(taskj);
    return AppendTask(t);
  } catch (const std::exception& e) {
    MYLOG_ERROR("[Edge:{}] AppendJsonTask 异常: {}", edge_id_, e.what());
    return false;
  }
}

bool BaseEdge::AppendTask(const my_data::Task& task) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_);

  // 允许外部把 self task 直接 append 进来（你要求外部可下发 self）
  if (task.device_id.empty()) {
    MYLOG_ERROR("[Edge:{}] AppendTask 失败: task.device_id 为空", edge_id_);
    return false;
  }

  std::string err;
  if (!AppendTaskToQueueLocked(task.device_id, task, &err)) {
    MYLOG_ERROR("[Edge:{}] AppendTask 失败: {}", edge_id_, err);
    return false;
  }

  MYLOG_INFO("[Edge:{}] AppendTask 成功: device_id={}, task_id={}", edge_id_, task.device_id, task.task_id);
  return true;
}

SubmitResult BaseEdge::MakeResult(SubmitCode code,
                                  const std::string& msg,
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

bool BaseEdge::AppendTaskToQueueLocked(const my_data::DeviceId& device_id,
                                      const my_data::Task& task,
                                      std::string* err) {
  auto qit = queues_.find(device_id);
  if (qit == queues_.end() || !qit->second) {
    if (err) *err = "找不到 device_id=" + device_id + " 对应的队列";
    return false;
  }
  if (qit->second->IsShutdown()) {
    if (err) *err = "队列已关闭，device_id=" + device_id;
    return false;
  }
  qit->second->Push(task);
  return true;
}

// ---------------- self action thread ----------------

void BaseEdge::StartSelfTaskMonitorThreadLocked() {
  if (!self_task_monitor_enable_) {
    MYLOG_INFO("[Edge:{}] self_task monitor 线程未启用", edge_id_);
    return;
  } else {
    MYLOG_INFO("[Edge:{}] self_task monitor 线程启用", edge_id_);
  }
  if (self_task_monitor_thread_.joinable()) {
    MYLOG_WARN("[Edge:{}] self_task monitor 线程已在运行", edge_id_);
    return;
  } else {
    MYLOG_INFO("[Edge:{}] self_task monitor 线程未在运行", edge_id_);
  }
  self_task_monitor_stop_.store(false);
  MYLOG_INFO("[Edge:{}] 启动 self_task monitor 线程", edge_id_);
  self_task_monitor_boot_at_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  MYLOG_INFO("[Edge:{}] self_task monitor 线程启动时间戳: {}", edge_id_, self_task_monitor_boot_at_ms_);
  self_task_monitor_thread_ = std::thread(&BaseEdge::SelfTaskMonitorLoop, this);
}


void BaseEdge::StopSelfTaskMonitorThreadLocked() {
  self_task_monitor_stop_.store(true);

  if (self_task_monitor_thread_.joinable()) {
    MYLOG_INFO("[Edge:{}] 等待 self_task monitor 线程退出...", edge_id_);
    self_task_monitor_thread_.join();
    MYLOG_INFO("[Edge:{}] self_task monitor 线程已退出", edge_id_);
  }
}

void BaseEdge::SelfTaskMonitorLoop() {
  MYLOG_INFO("[Edge:{}] self_task monitor loop 进入", edge_id_);

  while (!self_task_monitor_stop_.load()) {
    my_data::Task task;
    int wait_ms = 1000; // 退化行为，避免空转过快
    // 尝试获取任务（会在内部仅持短锁以查找队列），timeout_ms 与之前 sleep 行为一致
    int fetch_res = FetchSelfTask(task, wait_ms); // 1000ms 超时

    if (0 == fetch_res) { // No queue
      MYLOG_ERROR("[Edge:{}] self_action: self 队列不存在，线程退出, fetch_res: {}", edge_id_, fetch_res);
      return;
    }
    if (3 == fetch_res) { // Queue shutdown
      MYLOG_ERROR("[Edge:{}] self_action: self 队列已关闭，线程退出, fetch_res: {}", edge_id_, fetch_res);
      break;
    }
    if (5 == fetch_res) { // already has tas
      MYLOG_INFO("[Edge:{}] self_action: 已有未执行任务，继续等待 {} ms, fetch_res: {}", edge_id_, wait_ms, fetch_res);
    }
    if (2 == fetch_res) { // timeout, 无任务，继续循环
      MYLOG_INFO("[Edge:{}] self_action: 无任务，继续等待 {} ms, fetch_res: {}", edge_id_, wait_ms, fetch_res);

    }
    // 其它错误码（4）则回到循环重试
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms)); // 退化行为，避免空转过快
  }

  MYLOG_WARN("[Edge:{}] self_task monitor loop 退出", edge_id_);
  return;
}


void BaseEdge::StartSelfActionThreadLocked() {
  if (!self_action_enable_) {
    MYLOG_INFO("[Edge:{}] self_action 线程未启用", edge_id_);
    return;
  } else {
    MYLOG_INFO("[Edge:{}] self_action 线程启用", edge_id_);
  }
  if (self_action_thread_.joinable()) {
    MYLOG_WARN("[Edge:{}] self_action 线程已在运行", edge_id_);
    return;
  } else {
    MYLOG_INFO("[Edge:{}] self_action 线程未在运行", edge_id_);
  }
  self_action_stop_.store(false);
  MYLOG_INFO("[Edge:{}] 启动 self_action 线程", edge_id_);
  self_action_boot_at_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  MYLOG_INFO("[Edge:{}] self_action 线程启动时间戳: {}", edge_id_, self_action_boot_at_ms_);
  self_action_thread_ = std::thread(&BaseEdge::SelfActionLoop, this);
}

void BaseEdge::StopSelfActionThreadLocked() {
  self_action_stop_.store(true);

  // 尝试唤醒（如果 TaskQueue 没有 timed pop，这里只能依赖 Shutdown 时唤醒）
  // 这里不强依赖，join 前先 Shutdown 队列会更稳（但我们 shutdown 里是先停线程再 shutdown queue）
  // 所以建议 TaskQueue 支持带超时 pop；暂时在 loop 内 sleep 退化。

  if (self_action_thread_.joinable()) {
    MYLOG_INFO("[Edge:{}] 等待 self_action 线程退出...", edge_id_);
    self_action_thread_.join();
    MYLOG_INFO("[Edge:{}] self_action 线程已退出", edge_id_);
  }
}

void BaseEdge::SelfActionLoop() {
  MYLOG_INFO("[Edge:{}] self_action loop 进入", edge_id_);
  while (!self_action_stop_.load()) {
    ExecuteSelfTask();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 退化行为，避免空转过快
  }
  MYLOG_WARN("[Edge:{}] self_action loop 退出", edge_id_);
}

/**
 * @brief 尝试从 self 队列中获取任务，带超时
 * 
 * @param out 输出参数，获取到的任务
 * @param timeout_ms 超时时间，单位毫秒
 * @return int 状态码: 0=无队列，1=成功，2=超时，3=队列已关闭，4=错误，5=已有任务未执行
 */
int BaseEdge::FetchSelfTask(my_data::Task& out, int timeout_ms) {
  my_control::TaskQueue* q = nullptr;
  {
    std::shared_lock<std::shared_mutex> lk(rw_mutex_);
    auto it = queues_.find(self_device_id_);
    if (it == queues_.end() || !it->second) {
      return 0; // No queue
    }
    q = it->second.get();
  }

  if (q->IsShutdown()) return 3; // queue already shutdown

  bool ok = false;
  try {
    // 判断当前任务状态，避免重复 fetch
    RunState current_state = self_task_run_state_.load();
    if (current_state == RunState::Ready) {
      MYLOG_WARN("[Edge:{}] FetchSelfTask: 当前已有未执行任务，跳过本次 fetch", edge_id_);
      return 5; // already has task
    }
    if (current_state == RunState::Running) {
      MYLOG_WARN("[Edge:{}] FetchSelfTask: 当前任务正在运行，开始本次 fetch", edge_id_);
      return 5; // already has task
    }
    if (current_state == RunState::Initializing) {
      MYLOG_INFO("[Edge:{}] FetchSelfTask: 当前任务正在初始化，开始本次 fetch", edge_id_);
    }
    if (current_state == RunState::Stopping || current_state == RunState::Stopped) {
      MYLOG_WARN("[Edge:{}] FetchSelfTask: 当前处于停止状态，开始本次 fetch", edge_id_);
    }
    if (current_state == RunState::RunOver) {
      MYLOG_INFO("[Edge:{}] FetchSelfTask: 当前处于运行结束状态, 开始本次 fetch", edge_id_);
    }

    ok = q->PopBlocking(out, timeout_ms);
    if (!ok) {
      return 2; // timeout
    } else {
      {
        std::unique_lock<std::shared_mutex> lk(rw_mutex_self_task_);
        this->self_task = out; // 存储到成员变量，供 ExecuteSelfTask 使用
        this->self_task_run_state_.store(RunState::Initializing); // 标记为有新任务
        MYLOG_INFO("[Edge:{}] 更新当前任务, task_id={}, capability={}, action={}, params={}", edge_id_, out.task_id, out.capability, out.action, out.params.dump(4));
      }
    }
    
    
  } catch (const std::exception& e) {
    MYLOG_ERROR("[Edge:{}] FetchSelfTask 异常: {}", edge_id_, e.what());
    return 4; // error
  }

  if (!ok) {
    if (q->IsShutdown()) return 3; // shutdown triggered
    return 2; // timeout (no task)
  }
  return 1; // ok
}

void BaseEdge::ExecuteSelfTask() {
  ExecuteSelfTaskLocked();
}

void BaseEdge::ExecuteSelfTaskLocked() {
  if (self_task_run_state_.load() != RunState::Initializing) {
    // 无新任务
    return;
  }
  
  // if ("say_hello" == this->self_task.action) {
  //   SayHelloAction(this->self_task);
  // }
  {
    std::string action = this->self_task.action;
    SelfTaskHandler handler;
    {
        std::lock_guard<std::mutex> lk(self_task_handlers_mutex_);
        auto it = self_task_handlers_.find(action);
        if (it != self_task_handlers_.end()) {
            handler = it->second; // 拷贝回调对象到局部，随后在锁外调用
        }
    }
    if (handler) {
      MYLOG_INFO("-----------> Find {} call back.", action);
      handler(this->self_task);
    }
  }
  {
    // 标记任务已执行完毕
    std::unique_lock<std::shared_mutex> lk(rw_mutex_self_task_);
    this->self_task_run_state_.store(RunState::RunOver);
    MYLOG_INFO("[Edge:{}] self task 执行完毕，标记为 RunOver: task_id={}, capability={}, action={}",
               edge_id_, this->self_task.task_id, this->self_task.capability, this->self_task.action);
  }
  MYLOG_WARN("[Edge:{}] 未实现 self task 执行逻辑: capability={}, action={}", edge_id_, this->self_task.capability, this->self_task.action);
  return;
}

void BaseEdge::ExecuteOtherTask(const my_data::Task& task) {
  std::unique_lock<std::shared_mutex> lk(rw_mutex_self_task_);
  ExecuteOtherTaskLocked(task);
}

void BaseEdge::ExecuteOtherTaskLocked(const my_data::Task& task) {
  MYLOG_INFO("[Edge:{}] 执行 self task: task_id={}, capability={}, action={}, params={}",
             edge_id_, task.task_id, task.capability, task.action, task.params.dump(4));
  if ("say_hello" == task.action) {
    SayHelloAction(task);
    return;
  }
  // 你可以在这里实现一些内置动作示例: 
  // - capability="edge", action="ping" => 立即上报一次心跳
  // - capability="edge", action="estop" => SetEStop(true,...)
  // - capability="edge", action="clear_estop" => SetEStop(false,...)
  MYLOG_WARN("[Edge:{}] 未实现 self task 执行逻辑: capability={}, action={}", edge_id_, task.capability, task.action);
}

int BaseEdge::SayHelloAction(const my_data::Task& task) {
  if (RunState::Ready != this->self_task_run_state_.load() &&
      RunState::Initializing != this->self_task_run_state_.load()) {
    MYLOG_WARN("SayHelloAction 被拒绝执行: 当前 self_task_run_state={}",
               RunStateToString(this->self_task_run_state_.load()));
    return -2; // rejected
  }
  MYLOG_INFO("Hello from SayHelloAction! task_id={}", task.task_id);
  this->self_task_run_state_.store(RunState::Running); // 标记为有新任务
  for (int i = 0; i < 5; i++) {
    if (self_action_stop_.load()) {
      MYLOG_WARN("SayHelloAction 被请求停止！ task_id={}", task.task_id);
      this->self_task_run_state_.store(RunState::Stopped); // 标记任务
      return -1; // stopped
    }
    MYLOG_INFO("SayHelloAction 进行中... {}/5, task_id={}", i + 1, task.task_id);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 模拟工作
  }
  MYLOG_INFO("SayHelloAction completed! task_id={}", task.task_id);
  this->self_task_run_state_.store(RunState::RunOver); // 标记任务
  return 0; // success
}


void BaseEdge::StartSnapshotThreadLocked() {
  if (!snapshot_enable_) {
    MYLOG_INFO("[Edge:{}] snapshot/心跳线程未启用", edge_id_);
    return;
  }
  if (snapshot_thread_.joinable()) {
    MYLOG_WARN("[Edge:{}] snapshot/心跳线程已在运行", edge_id_);
    return;
  }
  snapshot_stop_.store(false);
  MYLOG_INFO("[Edge:{}] 启动 snapshot/心跳线程: interval_ms={}", edge_id_, snapshot_interval_ms_);
  snapshot_boot_at_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  MYLOG_INFO("[Edge:{}] snapshot/心跳线程启动时间戳: {}", edge_id_, snapshot_boot_at_ms_);
  snapshot_thread_ = std::thread(&BaseEdge::SnapshotLoop, this);

}

void BaseEdge::StopSnapshotThreadLocked() {
  snapshot_stop_.store(true);
  if (snapshot_thread_.joinable()) {
    MYLOG_INFO("[Edge:{}] 等待 snapshot/心跳线程退出...", edge_id_);
    snapshot_thread_.join();
    MYLOG_INFO("[Edge:{}] snapshot/心跳线程已退出", edge_id_);
  }
}

void BaseEdge::SnapshotLoop() {
  MYLOG_INFO("[Edge:{}] snapshot/心跳 loop 进入", edge_id_);

  while (!snapshot_stop_.load()) {
    {
      std::unique_lock<std::shared_mutex> lk(rw_mutex_);
      ReportHeartbeatLocked();
    }

    int interval = snapshot_interval_ms_;
    if (interval < 2002) {
        interval = 2002;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
  }

  MYLOG_WARN("[Edge:{}] snapshot/心跳 loop 退出", edge_id_);
}

void BaseEdge::ReportHeartbeatLocked() {
  // 默认只输出日志，后续你可以接入 MQTT/HTTP 上报
//   my_data::EdgeStatus st = GetStatusSnapshot();
//   MYLOG_INFO("[Edge:{}] 心跳: run_state={}, estop={}, pending_total={}, running_total={}",
//              edge_id_,
//              st.run_state == my_data::EdgeRunState::Running ? "Running" :
//              (st.run_state == my_data::EdgeRunState::EStop ? "EStop" : "Other"),
//              st.estop_active ? "true" : "false",
//              st.tasks_pending_total,
//              st.tasks_running_total);
}

void BaseEdge::RegisterSelfTaskHandler(const std::string& action, SelfTaskHandler handler) {
    std::lock_guard<std::mutex> lk(self_task_handlers_mutex_);
    self_task_handlers_[action] = std::move(handler);
}


} // namespace my_edge