# 设计文档：企业级边缘设备管理系统 (EdgeX-Manager)

### 1. 设计目标

* **高度解耦** ：`MyEdgeManager` 仅作为容器和协调器，具体的网络通信、在线自检、动作执行全部下沉到 `EdgeDevice` 类。
* **线程隔离** ：每个设备独立运行其监控线程，互不干扰，支持单设备精细化控制（启动/停止）。
* **状态透明化** ：实时维护并暴露设备的在线状态（Online）、业务姿态（Status）及线程运行健康度。

### 2. 核心组件描述

#### A. EdgeDevice 类 (Entity & Worker)

它是系统的最小功能单元，代表一个真实的硬件设备。

* **内部状态机** ：维护 `Offline`、`Online`、`Executing`、`Fault` 等状态。
* **在线检查器** ：集成 `DeviceOnlineMonitor`，通过心跳或 Socket 探测维持 `is_online` 字段。
* **私有线程池** ：每个实例内部管理一个独立的采集线程。
* **接口规范** ：提供 `start()`、`stop()`、`execute()`、`getStatus()` 接口供外部调用。

#### B. MyEdgeManager 类 (Orchestrator)

它是设备的集合管理者。

* **设备注册表** ：通过 `std::map<std::string, std::shared_ptr<EdgeDevice>>` 管理所有设备实例。
* **生命周期调度** ：全局初始化设备、批量启动、以及对特定设备下达指令。
* **信息聚合** ：汇总所有设备的状态，生成全局状态快照。

---

### 3. 关键业务流程

#### 3.1 设备初始化与在线自检

1. `MyEdgeManager` 解析 JSON。
2. 为每个配置项构造一个 `EdgeDevice` 实例。
3. `EdgeDevice` 内部调用 `DeviceOnlineMonitor::init()`。
4. 启动在线探测，若目标 IP (如 8.8.8.8) 及端口 (如 53) 响应，则标记 `online = true`。

#### 3.2 独立启停逻辑

* **单点启动** ：调用 `manager.startDevice("solar")`，仅激活该设备内部的 `worker_thread`。
* **单点停止** ：调用 `manager.stopDevice("solar")`，安全 join 该设备的线程，而不影响其他设备（如 "wind"）的工作。

#### 3.3 状态获取（Show Status）

* 调用 `manager.showEdgesStatus()`，Manager 遍历所有 Device 对象。
* 每个 Device 返回一个结构化对象（或 JSON），包含：
  * `name`, `ip`, `port`
  * `is_online` (网络层)
  * `business_status` (业务层，如：正在采集、空闲)
  * `thread_active` (系统层)

---

### 4. 接口契约设计 (Interface Contract)

#### MyEdgeManager 接口

| **接口名**                        | **参数**       | **功能描述**                              |
| --------------------------------------- | -------------------- | ----------------------------------------------- |
| `init(json)`                          | 配置对象             | 批量创建 EdgeDevice 实例并完成基础配置          |
| `startDevice(name)`                   | 设备名               | 单独启动某个设备的监控线程                      |
| `stopDevice(name)`                    | 设备名               | 单独停止某个设备的监控线程                      |
| `executeAction(name, action, params)` | 设备名, 动作名, 参数 | 穿透调用特定设备的业务接口                      |
| `getEdgesStatus()`                    | 无                   | 返回包含所有设备状态的 JSON 列表                |
| `getDeviceThread(name)`               | 设备名               | 返回该设备线程句柄或线程运行状态（如 NativeID） |

#### EdgeDevice 接口

| **接口名**            | **参数** | **功能描述**                           |
| --------------------------- | -------------- | -------------------------------------------- |
| `start()`                 | 无             | 激活内部 worker 线程，开始 getData 循环      |
| `stop()`                  | 无             | 设置原子退出信号，回收线程资源               |
| `getStatus()`             | 无             | 返回包含在线情况、业务姿态、运行指标的结构体 |
| `execute(action, params)` | 动作名, 参数   | 具体的网络协议实现（HTTP/TCP）               |

---

### 5. 异常处理策略 (Enterprise Standard)

* **资源竞争** ：在访问设备注册表 `device_map_` 时使用共享锁（Shared Mutex），支持多读一写。
* **监控自愈** ：若 `DeviceOnlineMonitor` 检测到断线，`EdgeDevice` 自动暂停业务采集逻辑，并记录 `MYLOG_ERROR`，直到网络恢复。
* **死锁预防** ：Device 内部不持有对 Manager 的引用，防止循环引用导致无法析构。
