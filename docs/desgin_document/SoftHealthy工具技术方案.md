# 软件健康监控模块设计文档（Soft Health Module）

## 一、模块目标

实现一个**可嵌入式监控模块**，用于周期性采集当前程序的运行健康信息，包括：

* 程序运行时长
* 内存使用情况（进程级和线程级）
* 线程总数及其父子关系
* CPU占用情况
* IO/上下文切换等资源使用情况

## 二、设计原则

* **低侵入性**：不会干扰主业务逻辑
* **线程安全**：多线程访问时不会产生竞态
* **模块化/可移植**：易于在不同项目中复用
* **可扩展性**：未来支持记录历史日志、上传监控数据

---

## 三、模块架构

```
SoftHealthMonitor
├── HealthMonitorManager         // 总控调度
├── ProcessInfoCollector         // 收集进程信息（内存、运行时间）
├── ThreadInfoCollector          // 收集线程信息（资源、关系）
├── ResourceUsageAnalyzer        // 分析资源使用占比
├── HealthSnapshot               // 当前快照（数据结构）
└── Reporter/Logger              // 数据输出接口
```

---

## 四、关键功能设计

### 1. HealthMonitorManager

* 启动一个线程（后台）
* 每隔 N 秒收集一次状态（默认 5 秒）
* 对接主程序生命周期（启动、退出）

### 2. ProcessInfoCollector（基于 `/proc/self/`）

* 进程启动时间（记录程序 start timestamp）
* 进程内存占用（`/proc/self/status` -> VmRSS、VmSize）
* CPU 占用率（通过`/proc/self/stat` + `/proc/stat`）
* 线程总数（`/proc/self/task/`子目录数量）

### 3. ThreadInfoCollector

* 遍历 `/proc/self/task/<tid>/` 获取每个线程
* 获取 thread name（`comm`）
* 获取 thread memory 估算（读取 `/proc/<pid>/task/<tid>/statm`）
* 获取 thread stack 信息（`/proc/<pid>/task/<tid>/stat`）

> ⚠️ 注意：Linux 下没有直接的 per-thread 内存指标，大多数线程共享进程空间，可尝试估算 stack 使用等。

### 4. Thread Parent-Child Relation

* 线程层级：可以通过 task 目录中“调度器信息”或 `pthread_getname_np` + 业务创建栈记录
* 或自定义包装线程创建接口保存父子信息（如用 wrapper 封装 `std::thread`）

### 5. ResourceUsageAnalyzer

* 将每个线程/进程的 CPU 时间（utime + stime）作差计算增量
* 计算出相对时间段内的 CPU 使用率
* 后期可扩展添加 IO、上下文切换、页错误信息

### 6. 输出方式（Reporter）

* 控制台输出 / 本地日志 / 远程发送
* 支持 JSON 格式方便集成其他系统

---

## 五、输出示例（JSON）

```json
{
  "uptime": "12345", 
  "memory": {
    "VmRSS_kB": 145920,
    "VmSize_kB": 412312
  },
  "threads": [
    {
      "tid": 1234,
      "name": "worker_thread",
      "cpu_usage_percent": 3.5,
      "stack_size_kB": 512,
      "parent_tid": 1
    },
    ...
  ]
}
```

---

## 六、线程安全与关闭控制

* 使用 `std::mutex` 保证数据更新安全
* 使用 `std::atomic<bool> running` 控制循环退出
* 提供 `start()` / `stop()` 接口用于模块控制

---

## 七、示意代码片段

```cpp
class SoftHealthMonitor {
public:
    void start();
    void stop();
    HealthSnapshot getSnapshot(); // 当前状态快照

private:
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    std::mutex data_mutex_;
    HealthSnapshot current_snapshot_;
};
```

---

## 八、依赖与适配

* 无外部依赖（纯 C++11 + Linux 系统文件）
* 支持 CMake 独立构建为 `.a` 或 `.so`
* 后续可扩展 Prometheus exporter、gRPC 接口等

---

## 九、后续扩展建议

* 内存泄露探测集成（如 jemalloc stats）
* 与主程序 crash handler 联动
* 对接系统 metrics 平台（如 Prometheus）
* 添加采集历史记录并输出图形

---

如果你需要，我可以帮你编写模块代码的初稿（如 `SoftHealthMonitor.h/.cpp` 和简单的输出逻辑），你要不要我接着写？
