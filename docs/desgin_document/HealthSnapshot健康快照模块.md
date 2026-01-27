
# 📄 健康快照模块：HealthSnapshot（最终整合版）

---

## 一、模块目标

用于描述程序当前时刻的完整运行快照，记录：

* 进程基本信息（程序名、PID、PPID、子进程）
* 进程内存使用（VmRSS、VmSize）
* 程序已运行时长
* 所有线程的资源使用情况（CPU、栈）
* 线程之间的父子关系树结构

---

## 二、数据结构定义（头文件）

### 📌 `HealthSnapshot.h`

```cpp


```

---

## 三、关键设计说明与注释

### ✅ `ThreadSnapshot`

| 字段名                 | 含义                                        |
| ------------------- | ----------------------------------------- |
| `tid`               | Linux 下的线程 ID（实际是一个轻量级进程）                 |
| `cpu_usage_percent` | 根据两次采样的 CPU 时间差计算得出                       |
| `stack_kb`          | 线程栈空间（通过 `/proc/self/task/<tid>/stat` 估算） |
| `parent_tid`        | 可以用包装器手动记录，系统不直接提供                        |

---

### ✅ `ProcessInfo`

来自 `/proc/self/status` 和 `/proc/self/comm` 的信息，覆盖以下字段：

* `pid`, `ppid`：获取进程层级关系
* `name`：获取当前执行程序名
* `children_pids`：遍历 `/proc/` 中所有 `ppid == pid` 的项得出
* `vm_size_kb`, `vm_rss_kb`：来自 `/proc/self/status`

---

### ✅ `ThreadTreeNode`

可选的**线程层级表示**，构建方式：

* 遍历线程列表，按 `parent_tid` 聚合
* 如果 `parent_tid == 0` 或不存在于其他线程，则为根节点

---

### ✅ `HealthSnapshot` 类方法

| 方法名                           | 功能说明                      |
| ----------------------------- | ------------------------- |
| `setUptime(start)`            | 用当前时间 - 启动时间，记录运行秒数       |
| `setThreadSnapshots(threads)` | 设置线程列表                    |
| `setProcessInfo(proc_info)`   | 设置进程数据                    |
| `buildThreadTree()`           | 构建线程树结构，便于关系图展示           |
| `toJsonString()`              | 导出为 JSON 字符串，便于调试/上报（未实现） |

---

## 四、后续任务建议

| 模块                     | 说明                                              |
| ---------------------- | ----------------------------------------------- |
| `HealthSnapshot.cpp`   | 实现上述各接口（含 `toJsonString()`）                     |
| `ThreadInfoCollector`  | 遍历 `/proc/self/task`，构造 `ThreadSnapshot`        |
| `ProcessInfoCollector` | 读取 `/proc/self/status`、`comm` 等填充 `ProcessInfo` |
| `Reporter`             | 输出或记录日志、推送 JSON                                 |
| `UnitTest`             | 写单元测试验证 `buildThreadTree()` 正确性                 |

---

## ✅ 结论

这个设计已经能支持你提出的目标场景 —— 进程/线程全局运行状态监控、展示、导出，结构合理、便于扩展。

如果你同意这个结构，下一步我可以：

* 写 `HealthSnapshot.cpp` 实现部分
* 写采集器 `ProcessInfoCollector.h/cpp` 和 `ThreadInfoCollector.h/cpp`
* 写 JSON 输出实现

请告诉我你希望我先做哪一步？或者是否直接进入 `.cpp` 实现部分？
