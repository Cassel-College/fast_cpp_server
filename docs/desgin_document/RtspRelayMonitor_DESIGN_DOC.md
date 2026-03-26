# RTSP 转发守护模块（RtspRelayMonitor）

# ✅ 一、模块设计说明（简洁版）

## 模块名称

RTSP 转发守护模块（RtspRelayMonitor）

## 模块职责

构建一个 **基于配置驱动的 RTSP 转发守护模块** ，负责：

* 解析 JSON 配置（enable + MediaMTX配置）
* 生成 MediaMTX 的 YAML 文件
* 检测 RTSP 源连通性
* 管理 MediaMTX 进程（启动 / 停止 / 重启）
* 提供运行状态（状态机）

---

# ✅ 二、目录结构

```text
rtsp_relay_monitor/
├── RtspRelayMonitor.h
├── RtspRelayMonitor.cpp
```

---

# ✅ 三、核心流程

## 1️⃣ 初始化（Init）

**输入：JSON配置**

```json
{
    "mediamtx_enable": true,
    "check_ip": "192.168.2.119",
    "mediamtx_json_config": {
        "rtspAddress": ":8555",
        "paths": {
            "live": {
                "source": "rtsp://192.168.2.119",
                "sourceOnDemand": true
            }
        }
    }
}
```

**内部处理：**

* 保存 JSON
* 解析 enable
* 提取 RTSP 源地址
* 构建 YAML 字符串
* 写入 mediamtx.yml（存在则删除重建）

---

## 2️⃣ 监控线程（核心循环）

```cpp
while (running_) {
    sleep(interval);

    if (!enable_) {
        StopRelayProcess();
        state_ = STOPPED;
        continue;
    }

    if (!CheckRtspSource()) {
        StopRelayProcess();
        state_ = SOURCE_LOST;
        continue;
    }

    if (!CheckProcessAlive()) {
        StartRelayProcess();
        state_ = STARTING;
    } else {
        state_ = RUNNING;
    }
}
```

---

## 3️⃣ 关键能力

* RTSP 连通性检测（socket）
* MediaMTX 进程检测（pid / 端口）
* 端口占用检测
* YAML 自动生成
* 进程守护（自恢复）

---

# ✅ 四、状态机

```cpp
enum class RtspRelayState {
    STOPPED,      // 未运行 / 未启用
    STARTING,     // 启动中
    RUNNING,      // 正常运行
    SOURCE_LOST,  // RTSP源不可达
    ERROR         // 异常
};
```

---

# ✅ 五、头文件（RtspRelayMonitor.h）

```cpp
#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

/**
 * @brief RTSP 转发守护模块
 *
 * 功能：
 * 1. 解析 JSON 配置
 * 2. 生成 MediaMTX YAML 配置文件
 * 3. 监控 RTSP 源连通性
 * 4. 管理 MediaMTX 进程（启动/停止/重启）
 */
class RtspRelayMonitor {
public:
    RtspRelayMonitor();
    ~RtspRelayMonitor();

    /**
     * @brief 初始化模块
     * @param jsonConfig JSON字符串（包含 enable + mediamtx 配置）
     * @return true 成功 / false 失败
     */
    bool Init(const std::string& jsonConfig);

    /**
     * @brief 启动监控线程
     */
    void Start();

    /**
     * @brief 停止模块
     */
    void Stop();

private:
    /* ================= 核心线程 ================= */

    /**
     * @brief 监控主循环
     */
    void MonitorLoop();

    /* ================= 功能函数 ================= */

    /**
     * @brief 检测 RTSP 源是否可达
     */
    bool CheckRtspSource();

    /**
     * @brief 检测 MediaMTX 进程是否运行
     */
    bool CheckProcessAlive();

    /**
     * @brief 启动转发进程（MediaMTX）
     */
    bool StartRelayProcess();

    /**
     * @brief 停止转发进程
     */
    void StopRelayProcess();

    /**
     * @brief 检测端口是否可用
     */
    bool IsPortAvailable(int port);

    /**
     * @brief 生成 YAML 文件
     */
    bool GenerateYAML();

private:
    /* ================= 配置 ================= */

    json json_config_;        // 原始JSON
    std::string yaml_content_;       // 生成的YAML字符串
    std::string yaml_file_name_ = "./mediamtx.yml"; // 默认路径
    std::string yaml_file_abs_path_ = "";

    std::string rtsp_source_;        // RTSP源地址
    int rtsp_port_ = 8555;           // 默认端口

    /* ================= 状态 ================= */

    std::atomic<bool> enable_{true};   // 是否启用（默认true）
    std::atomic<bool> running_{false}; // 线程运行标志

    /* ================= 线程 ================= */

    std::thread monitor_thread_;
    std::mutex mutex_;

    /* ================= 状态机 ================= */

    enum class RtspRelayState {
        STOPPED,
        STARTING,
        RUNNING,
        SOURCE_LOST,
        ERROR
    };

    RtspRelayState state_ = RtspRelayState::STOPPED;
};
```
