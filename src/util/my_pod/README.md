# 吊舱连接模块（Pod Connection Module）

## 概述

多厂商吊舱接入基础框架，提供统一的吊舱设备管理、能力注册、会话通信抽象。

## 架构设计

模块采用分层架构：

```
PodManager（管理层）
    └── BasePod（设备壳对象）
            ├── CapabilityRegistry（能力注册表）
            │       ├── IStatusCapability → BaseStatusCapability → DjiStatusCapability
            │       ├── IHeartbeatCapability → BaseHeartbeatCapability → DjiHeartbeatCapability
            │       ├── IPtzCapability → BasePtzCapability → DjiPtzCapability / PinlingPtzCapability
            │       ├── ILaserCapability → BaseLaserCapability → DjiLaserCapability / PinlingLaserCapability
            │       ├── IStreamCapability → BaseStreamCapability → DjiStreamCapability
            │       ├── IImageCapability → BaseImageCapability → DjiImageCapability
            │       └── ICenterMeasureCapability → BaseCenterMeasureCapability → DjiCenterMeasureCapability
            └── ISession → BaseSession → DjiSession / PinlingSession
```

## 核心设计原则

1. **BasePod 是轻量设备壳对象** — 只负责设备信息、生命周期、Session 持有和能力注册
2. **Capability 是核心扩展点** — 每种能力独立建模（接口→基类→厂商实现）
3. **Vendor Pod 尽量轻** — 只做 Session 初始化和能力注册装配
4. **CapabilityRegistry 管理能力** — 统一注册、查询、枚举
5. **Session 单独抽象** — 能力依赖 Session 通信，不直接实现网络细节

## 目录结构

```
src/util/my_pod/
├── CMakeLists.txt
├── common/          # 基础类型、模型、错误码
├── pod/             # 吊舱设备层（IPod → BasePod → DjiPod/PinlingPod）
├── capability/      # 能力层（接口 → 基类 → 厂商实现）
├── session/         # 会话层（ISession → BaseSession → 厂商Session）
├── registry/        # 注册表（CapabilityRegistry, PodRegistry）
├── manager/         # 管理层（PodManager, PodScheduler, PodEventDispatcher）
└── utils/           # 工具类
```

## 快速使用

```cpp
#include "pod/dji/dji_pod.h"
#include "manager/pod_manager.h"
#include "capability/interface/i_ptz_capability.h"

// 1. 创建吊舱
PodModule::PodInfo info;
info.pod_id = "dji_001";
info.pod_name = "大疆吊舱1号";
info.vendor = PodModule::PodVendor::DJI;
info.ip_address = "192.168.1.100";
info.port = 8080;

auto dji_pod = std::make_shared<PodModule::DjiPod>(info);

// 2. 初始化能力
dji_pod->initializeCapabilities();

// 3. 连接设备
dji_pod->connect();

// 4. 使用能力
auto ptz = dji_pod->getCapabilityAs<PodModule::IPtzCapability>(PodModule::CapabilityType::PTZ);
if (ptz) {
    auto pose = ptz->getPose();
    // ...
}

// 5. 使用管理器统一管理
PodModule::PodManager manager;
manager.addPod(dji_pod);
```

## 支持的能力

| 能力类型 | 描述 | 接口 |
|---------|------|------|
| STATUS | 状态查询 | `IStatusCapability` |
| HEARTBEAT | 心跳检测 | `IHeartbeatCapability` |
| PTZ | 云台控制 | `IPtzCapability` |
| LASER | 激光测距 | `ILaserCapability` |
| STREAM | 流媒体 | `IStreamCapability` |
| IMAGE | 图像抓拍 | `IImageCapability` |
| CENTER_MEASURE | 中心点测量 | `ICenterMeasureCapability` |

## 扩展指南

### 添加新厂商

1. 在 `session/` 下新建厂商 Session（继承 `BaseSession`）
2. 在 `capability/` 下新建厂商能力实现（继承 `Base*Capability`）
3. 在 `pod/` 下新建厂商 Pod（继承 `BasePod`），在 `initializeCapabilities()` 中注册能力

### 添加新能力

1. 在 `capability/interface/` 下定义新能力接口（继承 `ICapability`）
2. 在 `capability/base/` 下实现基础版本（继承 `BaseCapability` 和新接口）
3. 在 `common/capability_types.h` 中添加能力类型枚举
4. 各厂商按需实现具体能力
