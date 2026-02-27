#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "IEdge.h"
#include "MyData.h"
#include "MyLog.h"
#include "IDevice.h"
#include "TaskQueue.h"
#include "demo/Task.h"

namespace my_edge {

/**
 * @brief BaseEdge: 实现 IEdge 的通用骨架
 *
 * 设计目标: 
 * 1) Edge 自身具备执行能力: 内置 self 队列 + self_action 线程
 * 2) device 是可选的: cfg.devices 可以为空/缺失
 * 3) 心跳/上报线程独立: snapshot_thread_ 负责心跳上报（也可扩展为状态上报）
 * 4) Submit/Init/Start/Shutdown 的并发与状态机统一
 *
 * 子类只需要关注差异点: 
 * - Normalize 策略（缓存/临时创建 normalizer 等）
 * - self task 执行策略（覆盖 ExecuteSelfTaskLocked）
 * - edge_type / 默认 edge_id / 扩展状态字段等
 */
class BaseEdge : public IEdge {
public:
  BaseEdge();
  explicit BaseEdge(std::string edge_type);
  ~BaseEdge() override;

  // IEdge 实现
  bool Init(const nlohmann::json& cfg, std::string* err) override;          // 负责解析 cfg 和初始化成员变量，但不启动设备线程
  bool Start(std::string* err) override;                                    // 负责启动设备线程、self action 线程和 snapshot 线程，切换 run_state 到 Running
  SubmitResult Submit(const my_data::RawCommand& cmd) override;             // 负责 run_state/estop/device_id 校验，调用 NormalizeCommandLocked 获取 Task，并分发到对应队列
  my_data::EdgeStatus GetStatusSnapshot() const override;                   // 负责收集基本状态信息（run_state、estop、队列长度等），并调用 ReportHeartbeatLocked 上报
  void SetEStop(bool active, const std::string& reason) override;           // 负责设置 estop 状态和原因，并在日志中记录
  void Shutdown() override;                                                 // 负责停止设备线程、self action 线程和 snapshot 线程，清理资源，切换 run_state 到 Stopped
  my_data::EdgeId Id() const override;                                      // 负责返回 edge_id        
  std::string EdgeType() const override;                                    // 负责返回 edge_type
  void ShowAnalyzeInitArgs(const nlohmann::json& cfg) const override;       // 负责解析 Init 入参并记录到日志
  nlohmann::json DumpInternalInfo() const override;                         // 负责输出内部状态信息（受 rw_mutex_ 保护）
  nlohmann::json GetRunTimeStatusInfo() const override;                     // 负责输出运行时状态信息（供 DumpInternalInfo 调用）
  bool AppendJsonTask(const nlohmann::json& task) override;                 // 负责把 JSON 任务转换成 Task 并调用 AppendTask
  bool AppendTask(const my_data::Task& task) override;                      // 负责把 Task 分发到对应队列（受 rw_mutex_ 保护）  

  // Self task 回调签名: task_id, capability, action, params（均为 string，按需修改）
  using SelfTaskHandler = std::function<void(const my_data::Task& task)>;

  // 注册回调（线程安全）: action -> handler
  void RegisterSelfTaskHandler(const std::string& action, SelfTaskHandler handler);
protected:
  // -------- 子类需要实现/可覆盖的钩子 --------

  /**
   * @brief Normalize: 把 RawCommand 转成 Task。
   * 注意: BaseEdge 已做 run_state/estop/payload/device_id 校验，这里只负责“生成 task”。
   *
   * @return 失败返回 nullopt，并在 err 中写入原因。
   */
  virtual std::optional<my_data::Task> NormalizeCommandLocked(const my_data::RawCommand& cmd,
                                                              const my_data::DeviceId& device_id,
                                                              const std::string& device_type,
                                                              std::string* err) = 0;

  /**
   * @brief self task 执行入口（可由子类扩展）
   * 默认实现会打印日志，并根据 capability/action 做少量内置动作。
   */
  virtual void ExecuteOtherTaskLocked(const my_data::Task& task);
  virtual void ExecuteSelfTaskLocked();

  /**
   * @brief 心跳上报（snapshot thread 周期调用）
   * 默认实现: 只打日志。后续你可以接入 MQTT/HTTP 等上报。
   */
  virtual void ReportHeartbeatLocked();

protected:
  // -------- 受 rw_mutex_ 保护的成员 --------
  mutable std::shared_mutex rw_mutex_;
  my_data::EdgeId           edge_id_{"edge-default"};                       // Edge ID（可由子类覆盖）
  std::string               edge_type_{"base"};                             // Edge 类型（可由子类覆盖）
  std::string               version_{"0.0"};                                // 版本信息（可由子类覆盖）
  nlohmann::json            cfg_;                                           // 原始配置（Init 入参） 
  std::int64_t              boot_at_ms_{0};                                 // 启动时间戳
  std::atomic<RunState>     run_state_{RunState::Initializing};             // 运行状态
  std::atomic<bool>         estop_{false};                                  // EStop 状态
  mutable std::mutex        estop_mu_;                                      // 保护 estop_
  std::string               estop_reason_{};                                // EStop 原因
  bool                      allow_queue_when_estop_{false};                 // 默认不允许在 EStop 时入队
  DeviceID_Type___Mapping   device_type_by_id_;                             // device_id -> device_type
  DeviceID_Tasks__Mapping   queues_;                                        // device_id -> TaskQueue ptr
  DeviceID_Device_Mapping   devices_;                                       // device_id -> IDevice ptr
  std::string               self_device_id_{"self"};                        // edge 自己的 device_id
  my_data::Task             self_task;                                      // 用于 self action 线程的临时任务存储
  std::atomic<bool>         self_task_executing_{false};                    // self task 执行状态
  mutable std::shared_mutex rw_mutex_self_task_;                            // 保护 self_task_
  std::atomic<RunState>     self_task_run_state_{RunState::Initializing};   // self task 执行状态
  bool                      self_task_monitor_enable_{true};                // 启动监控Task 默认启用
  std::atomic<bool>         self_task_monitor_stop_{false};                 // self_task_monitor 线程停止标志 
  std::thread               self_task_monitor_thread_;                      // self_task_monitor 线程 
  std::int64_t              self_task_monitor_boot_at_ms_{0};               // self_task_monitor 启动时间戳  
  bool                      self_action_enable_{true};                      // 启动执行Task 默认启用
  std::atomic<bool>         self_action_stop_{false};                       // self action 线程停止标志 
  std::thread               self_action_thread_;                            // self action 线程 
  std::int64_t              self_action_boot_at_ms_{0};                     // self action 启动时间戳
  bool                      snapshot_enable_{true};                         // 默认启用
  int                       snapshot_interval_ms_{2000};                    // 默认 2s 心跳
  std::atomic<bool>         snapshot_stop_{false};                          // snapshot 线程停止标志
  std::thread               snapshot_thread_;                               // snapshot 线程
  std::int64_t              snapshot_boot_at_ms_{0};                        // snapshot 启动时间戳

protected:
  // -------- 线程相关（锁内调用） --------

  void StartSelfTaskMonitorThreadLocked();
  void StopSelfTaskMonitorThreadLocked();
  void SelfTaskMonitorLoop();

  /**
   * @brief 从 self 队列获取任务
   * @param out 输出任务
   * @param timeout_ms 超时时间（毫秒）
   * @return 状态码: 0 = 没有队列（NoQueue），1 = 获取成功（OK），2 = 超时（Timeout），3 = 队列已关闭（Shutdown），4 = 错误（Error）5=已有任务未执行
   */
  int FetchSelfTask(my_data::Task& out, int timeout_ms = 500);

  void StartSelfActionThreadLocked();
  void StopSelfActionThreadLocked();
  void SelfActionLoop();

  /**
   * @brief 执行其他任务
   * @param task 要执行的任务
   */
  void ExecuteOtherTask(const my_data::Task& task);

  /**
   * @brief 执行 self 任务
   */
  void ExecuteSelfTask();

  /**
   * @brief 启动 snapshot 线程（锁内调用）
   */
  void StartSnapshotThreadLocked();

  /**
   * @brief 停止 snapshot 线程（锁内调用）
   */
  void StopSnapshotThreadLocked();

  /**
   * @brief snapshot 线程主循环
   */
  void SnapshotLoop();

  /**
   * @brief 内置 self action 示例: 打印 Hello
   * 
   * @param task 任务
   * @return int 状态码
   */
  int SayHelloAction(const my_data::Task& task);

  // -------- 通用工具函数 --------
  SubmitResult MakeResult(SubmitCode code,
                          const std::string& msg,
                          const my_data::RawCommand& cmd,
                          const my_data::DeviceId& device_id = "",
                          const my_data::TaskId& task_id = 0,
                          std::int64_t queue_size_after = 0) const;

  bool AppendTaskToQueueLocked(const my_data::DeviceId& device_id, const my_data::Task& task, std::string* err);

private:
  std::unordered_map<std::string, SelfTaskHandler> self_task_handlers_;
  std::mutex self_task_handlers_mutex_;
};

} // namespace my_edge