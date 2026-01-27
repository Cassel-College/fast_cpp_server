# SystemHealthy 工具技术方案

## 一、目标

构建一个 SystemHealthy 工具，用于检测应用软件所运行硬件环境的性能和健康状态。

## 二、需要监控的硬件指标

| 指标       | 含义                                     | 获取方式 (Linux)                      |
| ---------- | ---------------------------------------- | ------------------------------------- |
| CPU 利用率 | 服务器处理器处事动态                     | /proc/stat 分析                       |
| CPU 核心数 | 逻辑核数量与每核使用率                   | /proc/stat 每个 core 分析             |
| 内存占用率 | 内存总量、已使用、空闲、buffer/cache     | /proc/meminfo 分析                    |
| 磁盘 IO    | 磁盘读写的数据量 / IO时间                | /proc/diskstats， iostat (sysstat)    |
| 网络通信   | 每秒网络输入输出量                       | /proc/net/dev， ifconfig 进行计算     |
| 运行时间   | 系统运行的总时长                         | uptime 或 /proc/uptime                |
| 清净压力   | load average / CPU core数比较            | uptime 输出解析 load average          |
| GPU 占用   | 如果有GPU，监控GPU资源                   | nvidia-smi，drm 接口，gpu_top (intel) |
| 系统平台   | 操作系统平台名称，如 Linux/Windows/MacOS | uname 或 lsb_release -a               |
| CPU 平台   | x86_64、armv8 等架构信息                 | uname -m，lscpu                       |
| 进程监控   | 当前所有进程名与其资源占用情况           | /proc/PID/status, top，ps 等          |

## 三、proto 数据结构定义

```
syntax = "proto3";

package SystemHealthy;

message CPUCoreInfo {
  int32 core_id = 1;
  float usage_percent = 2;
}

message CPUInfo {
  string architecture = 1;
  int32 core_count = 2;
  repeated CPUCoreInfo cores = 3;
}

message MemInfo {
  uint64 total_bytes = 1;
  uint64 available_bytes = 2;
  float usage_percent = 3;
}

message DiskInfo {
  uint64 read_bytes = 1;
  uint64 write_bytes = 2;
}

message NetInfo {
  uint64 recv_bytes = 1;
  uint64 transmit_bytes = 2;
}

message GPUInfo {
  string vendor = 1;
  float usage_percent = 2;
}

message ProcessInfo {
  int32 pid = 1;
  string name = 2;
  float cpu_percent = 3;
  float mem_percent = 4;
}

message SystemInfo {
  string os_name = 1;
  string platform = 2;
  uint64 uptime_seconds = 3;
  CPUInfo cpu_info = 4;
  MemInfo mem_info = 5;
  DiskInfo disk_info = 6;
  NetInfo net_info = 7;
  repeated GPUInfo gpu_infos = 8;
  repeated ProcessInfo processes = 9;
}
```

## 四、说明文档

- `SystemInfo` 是总数据结构，用于存放整个系统信息快照。
- `CPUInfo` 包括整体架构、核数量、每个核的使用情况（`CPUCoreInfo`）
- `MemInfo` 存储内存总量与使用比
- `DiskInfo`、`NetInfo`：分别是磁盘和网络 IO
- `GPUInfo` 可根据平台采集多个 GPU 状态
- `ProcessInfo`：每个进程名、占用资源情况（CPU、内存百分比）

## 五、集成与扩展

1. 可通过 protobuf 序列化系统状态到文件 / 网络上传
2. 若系统中没有 GPU 或某项指标，可选择性为空
3. 提供一个采集器模块，定时填充 `SystemInfo` 实例并导出 JSON 或 proto buffer

## 六、后续建议

- 可扩展字段如温度、风扇转速、系统负载、线程数量等
- 跨平台适配：Windows 下使用 `GetSystemInfo` 等 API 实现兼容

如果你需要，我可以继续提供该 proto 的生成代码和单元测试实现。