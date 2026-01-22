
# MQTT 模块设计文档 (Design Document)

## 1. 概述

`fast_mqtt` 模块是一个基于 `libmosquitto` 的 C++ 封装库。它为服务端程序提供了嵌入式 MQTT 代理（Broker）管理、多客户端管理以及高可用单例客户端的实现。

## 2. 架构设计

系统采用三层架构设计：

* **服务层 (Server Layer)** : `MFMQTTServer` 负责本地 Mosquitto 进程的生命周期管理（配置生成、启动、停止）。
* **管理层 (Management Layer)** : `MFMQTTManager` 维护多个独立客户端，适用于需要连接多个不同 Broker 的场景。
* **应用层 (Singleton Layer)** : `MFSMQTTClient` 提供全局唯一访问点，简化了最常用的单 Broker 连接需求。

## 3. 核心组件说明

### 3.1 MFMQTTClient (基础类)

* **非阻塞 I/O** : 封装了 `mosquitto_loop_start`，在独立线程中处理网络事件。
* **连接健壮性** : 实现了 **指数退避算法 (Exponential Backoff)** 。当连接断开时，重连间隔从 2s 开始倍增，最高上限 32s，避免对 Broker 造成冲击。
* **线程安全** : 使用 `std::mutex` 保护发布（Publish）操作，确保多线程调用安全。

### 3.2 MFSMQTTClient (单例代理)

* **懒汉式单例** : 确保全局唯一性。
* **接口转发** : 内部持有 `std::shared_ptr<MFMQTTClient>`，将业务逻辑透明转发至基础类。

### 3.3 MFMQTTServer (代理管理)

* **动态配置** : 根据传入的 JSON 对象动态生成物理 `mosquitto.conf`。
* **进程监控** : 使用 `fork` 和 `execlp` 启动守护进程，并支持 `SIGTERM` 优雅关闭。

## 4. 关键算法实现：自动重连

代码通过 `on_disconnect` 回调触发 `schedule_reconnect`。

1. 计算当前等待时间：**$wait\_sec = \min(current\_backoff, max\_backoff\_sec)$**。
2. 开启异步线程进行 `sleep`。
3. 调用 `mosquitto_reconnect_async` 尝试恢复。
