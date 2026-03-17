# MyFlyControl 飞控通信模块 — 设计文档与使用手册

> 版本: 1.1  
> 日期: 2026-03-17  
> 参考协议: 域控飞控通信协议v1.0_20260313

---

## 目录

1. [模块定位](#1-模块定位)
2. [架构设计](#2-架构设计)
3. [分层详解](#3-分层详解)
4. [协议数据结构](#4-协议数据结构)
5. [帧格式说明](#5-帧格式说明)
6. [公共接口说明](#6-公共接口说明)
7. [JSON 配置格式](#7-json-配置格式)
8. [使用示例](#8-使用示例)
9. [CMake 集成](#9-cmake-集成)
10. [单元测试](#10-单元测试)
11. [线程安全设计](#11-线程安全设计)
12. [已知限制与后续扩展](#12-已知限制与后续扩展)

---

## 1. 模块定位

`my_fly_control` 是域控系统与飞控之间的串口通信模块，负责：

- **解析飞控心跳数据** — 实时获取飞控状态（位置、姿态、电池、故障等）
- **发送飞行控制指令** — 航线设置、速度/高度控制、开伞、末制导等
- **帧层协议管理** — 帧同步、校验和验证、粘包/半包处理
- **指令回复处理** — 接收飞控的指令执行结果

### 依赖关系

```
my_fly_control
├── my_serial        (串口读写)
├── my_serial_core   (wjwwood/serial 底层库)
├── my_tools         (MyLEHelper 小端编解码)
├── mylog            (日志)
└── nlohmann_json    (JSON 配置)
```

---

## 2. 架构设计

模块采用五层架构，自下而上：

```
┌─────────────────────────────────────┐
│  管理层 (MyFlyControlManager)       │  ← 单例入口
│  · GetInstance 全局唯一访问点        │
│  · Init/Start/Stop/Shutdown         │
│  · 生命周期守卫与实例重建           │
├─────────────────────────────────────┤
│  业务层 (MyFlyControl)              │  ← 对外接口
│  · Init/Start/Stop 生命周期          │
│  · SendSetRoute/GetLatestHeartbeat  │
│  · 回调注册与状态缓存               │
├─────────────────────────────────────┤
│  协议编解码层 (FlyControlCodec)      │
│  · Encode* — 结构体→帧字节          │
│  · Decode* — 帧载荷→结构体          │
│  · 使用 MyLEHelper 编解码            │
├─────────────────────────────────────┤
│  帧层 (FlyControlFrame)             │
│  · BuildFrame — 组装完整帧           │
│  · FrameParser — 字节流拆帧          │
│  · CalcChecksum — 校验和计算         │
├─────────────────────────────────────┤
│  传输层 (MySerial + MyLEHelper)     │  ← 已有模块
│  · 串口读写                          │
│  · 小端编解码                        │
└─────────────────────────────────────┘
```

---

## 3. 分层详解

### 3.1 帧层（FlyControlFrame）

**职责**：在连续字节流中识别完整帧边界，验证数据完整性。

**核心能力**：
- **帧同步** — 在字节流中搜索帧头 `0xEB 0x90`，自动丢弃脏数据
- **长度确定** — 根据帧类型和指令类型计算预期帧总长度
- **变长帧** — 航线帧和电子围栏帧可动态计算长度
- **校验和验证** — 帧头到校验和前所有字节求和取低8位
- **粘包/半包** — 使用 `std::deque` 缓冲区，支持分批喂入数据

**关键 API**：
| 函数 | 说明 |
|------|------|
| `CalcChecksum(data, len)` | 计算校验和 |
| `BuildFrame(cnt, type, payload)` | 组装完整帧 |
| `FrameParser::FeedData(data)` | 喂入字节数据 |
| `FrameParser::PopFrame(frame)` | 取出一个完整帧 |
| `FrameParser::Reset()` | 重置解析器 |

### 3.2 协议编解码层（FlyControlCodec）

**职责**：将协议数据结构与原始字节互相转换。

**编码方向（域控→飞控）**：
- `Encode*` 函数将结构体序列化为载荷，再调用 `BuildFrame` 组装完整帧
- 全部使用 `MyLEHelper` 写入小端字节

**解码方向（飞控→域控）**：
- `Decode*` 函数从 `ParsedFrame::payload` 中用 `MyLEHelper` 按序读出字段
- 心跳帧、云台控制帧、指令回复帧均有对应解码函数

### 3.3 业务层（MyFlyControl）

**职责**：提供高级 API，管理串口连接和收发线程。

**核心设计**：
- **后台接收线程** — `Start()` 后启动独立线程持续读取串口数据
- **帧解析分发** — 接收线程将数据喂入 `FrameParser`，取出帧后按类型分发
- **状态缓存** — 维护最新心跳数据 `latest_hb_`，带 mutex 保护
- **回调通知** — 支持注册心跳/回复/云台控制三种回调
- **发送互斥** — 所有发送操作共享 `send_mutex_` 防止并发写串口

### 3.4 管理层（MyFlyControlManager）

**职责**：为应用层提供“单例风格”的统一入口，同时不破坏底层 `MyFlyControl` 的可实例化能力。

**核心设计**：
- **单例入口** — 通过 `GetInstance()` 提供进程内唯一访问点，便于业务层统一调用
- **生命周期守卫** — 在 `Start()`、`Send*()` 前检查是否已经 `Init()`，给出更明确的错误信息
- **安全重建** — `Shutdown()` 时停止旧对象并重建一个新的 `MyFlyControl`，清空旧回调和旧状态
- **兼容扩展** — 上层若只需要一个飞控，使用 Manager；若后续需要多飞控，直接继续实例化 `MyFlyControl`

---

## 4. 协议数据结构

### 4.1 接收类结构体（飞控→域控）

| 结构体 | 帧类型 | 说明 |
|--------|--------|------|
| `HeartbeatData` | 0x01 | 飞控状态心跳（89字节帧，500ms周期） |
| `GimbalControlData` | 0x30 | 飞控控制云台转动指令 |
| `CommandReplyData` | 0x20 | 指令执行结果回复 |

### 4.2 发送类结构体（域控→飞控）

| 结构体 | 帧类型 | 指令 | 说明 |
|--------|--------|------|------|
| `SetDestinationData` | 0x02 | 0x01 | 设置飞行目的地 |
| `SetRouteData` | 0x10 | 0x02 | 设置飞行航线（变长） |
| `SetAngleData` | 0x10 | 0x03 | 角度控制 |
| `SetSpeedData` | 0x10 | 0x04 | 速度控制 |
| `SetAltitudeData` | 0x10 | 0x05 | 高度控制 |
| `PowerSwitchData` | 0x10 | 0x06 | 载荷电源开关 |
| `ParachuteData` | 0x10 | 0x07 | 开伞控制 |
| `ButtonCommandData` | 0x10 | 0x09 | 指令按钮 |
| `SetOriginReturnData` | 0x10 | 0x0A | 设置原点/返航点 |
| `SetGeofenceData` | 0x10 | 0x0B | 设置电子围栏（变长） |
| `SwitchModeData` | 0x10 | 0x0C | 切换运行模式 |
| `GuidanceData` | 0x10 | 0x0D | 末制导指令 |
| `GuidanceNewData` | 0x10 | 0x0E | 末制导指令（新版） |
| `GimbalAngRateData` | 0x10 | 0x0F | 吊舱姿态-角速度模式 |
| `GimbalAngleData` | 0x10 | 0x10 | 吊舱姿态-角度模式 |
| `TargetStateData` | 0x10 | 0x11 | 识别目标状态 |

### 4.3 辅助类型

| 类型 | 说明 |
|------|------|
| `FaultBits` | 故障信息位解析（16个bit位） |
| `RunMode` | 运行模式枚举 |
| `FlightMode` | 飞行模式枚举 |
| `FlightState` | 飞行状态枚举（24种状态） |
| `RoutePoint` | 航线点（经纬度+高度+序号+速度） |
| `GeofencePoint` | 电子围栏点（经纬度+高度+序号） |
| `ParsedFrame` | 帧层解析输出结构 |

---

## 5. 帧格式说明

### 通用帧格式

```
┌────────┬─────┬────────┬──────────┬──────┬────────┐
│ 帧头(2)│CNT(1)│帧类型(1)│ 载荷(N)  │校验(1)│ 帧尾(2)│
│ EB 90  │     │        │          │      │ ED DE  │
└────────┴─────┴────────┴──────────┴──────┴────────┘
```

- **帧头**: `0xEB 0x90`（固定2字节）
- **CNT**: 帧计数器（1字节，自增）
- **帧类型**: 区分帧类别（1字节）
- **载荷**: 具体数据（长度因帧类型而异）
- **校验和**: 帧头到载荷末尾所有字节求和取低8位
- **帧尾**: `0xED 0xDE`（固定2字节）

### 字节序

**小端模式（低字节在前）**，使用 `MyLEHelper` 编解码。

### 各帧长度

| 帧类型 | 固定/变长 | 总长度 |
|--------|-----------|--------|
| 心跳帧 0x01 | 固定 | 89 字节 |
| 目的地帧 0x02 | 固定 | 18 字节 |
| 回复帧 0x20 | 固定 | 9 字节 |
| 云台帧 0x30 | 固定 | 12 字节 |
| 指令帧 0x10 | 视指令而定 | 见各指令 |

---

## 6. 公共接口说明

模块现在提供两套对外接口：

- **MyFlyControlManager** — 推荐给应用层使用，单例入口更适合系统级调用
- **MyFlyControl** — 推荐给库层或未来多飞控场景使用，保留多实例能力

### 6.1 MyFlyControlManager 生命周期

```cpp
static MyFlyControlManager& GetInstance();

bool Init(const nlohmann::json& cfg, std::string* err = nullptr);
bool Start(std::string* err = nullptr);
void Stop();
void Shutdown();
bool IsInitialized() const;
bool IsRunning() const;
```

### 6.2 MyFlyControl 生命周期

```cpp
// 初始化
bool Init(const nlohmann::json& cfg, std::string* err = nullptr);

// 启动（开启后台接收线程）
bool Start(std::string* err = nullptr);

// 停止（关闭线程和串口）
void Stop();

// 是否运行中
bool IsRunning() const;
```

### 6.3 回调注册

```cpp
void SetOnHeartbeat(HeartbeatCallback cb);       // 心跳更新
void SetOnCommandReply(CommandReplyCallback cb);  // 指令回复
void SetOnGimbalControl(GimbalControlCallback cb);// 云台控制
```

> 回调接口在 `MyFlyControlManager` 与 `MyFlyControl` 中保持一致，便于无缝切换。

### 6.4 状态查询

```cpp
HeartbeatData  GetLatestHeartbeat() const;      // 获取最新心跳
nlohmann::json GetLatestHeartbeatJson() const;   // JSON 格式心跳
FaultBits      GetFaultBits() const;             // 故障位解析
bool           HasHeartbeat() const;             // 是否收到过心跳
```

### 6.5 发送指令

```cpp
bool SendSetDestination(int32_t lon, int32_t lat, uint16_t alt, ...);
bool SendSetRoute(const std::vector<RoutePoint>& points, ...);
bool SendSetAngle(int16_t pitch, uint16_t yaw, ...);
bool SendSetSpeed(uint16_t speed, ...);
bool SendSetAltitude(uint8_t alt_type, uint16_t altitude, ...);
bool SendPowerSwitch(uint8_t command, ...);
bool SendParachute(uint8_t parachute_type, ...);
bool SendButtonCommand(uint8_t button, ...);
bool SendSetOriginReturn(uint8_t point_type, int32_t lon, int32_t lat, uint16_t alt, ...);
bool SendSetGeofence(const std::vector<GeofencePoint>& points, ...);
bool SendSwitchMode(uint8_t mode, ...);
bool SendGuidance(const GuidanceData& data, ...);
bool SendGuidanceNew(const GuidanceNewData& data, ...);
bool SendGimbalAngRate(const GimbalAngRateData& data, ...);
bool SendGimbalAngle(const GimbalAngleData& data, ...);
bool SendTargetState(const TargetStateData& data, ...);
```

> 所有 Send* 函数返回 `true` 表示发送成功，可选的 `err` 参数获取错误信息。

---

## 7. JSON 配置格式

传给 `Init()` 的 JSON 直接传递给 MySerial，配置示例：

```json
{
    "port": "/dev/ttyS1",
    "baudrate": 115200,
    "timeout_ms": 100,
    "inter_byte_timeout_ms": 50,
    "read_timeout_constant_ms": 100,
    "read_timeout_multiplier_ms": 0,
    "write_timeout_constant_ms": 100,
    "write_timeout_multiplier_ms": 0,
    "bytesize": "eightbits",
    "parity": "none",
    "stopbits": "one",
    "flowcontrol": "none",
    "auto_open": true
}
```

也兼容设备侧常见写法：

```json
{
    "device": "/dev/ttyS1",
    "baud_rate": 115200,
    "timeout_ms": 100,
    "data_bits": 8,
    "parity": "none",
    "stop_bits": 1,
    "flow_control": "none"
}
```

> 飞控协议要求：1个起始位，8个数据位，1个停止位，无校验。上述配置与之匹配。

---

## 8. 使用示例

### 8.1 推荐用法：通过管理器单例访问

```cpp
#include "MyFlyControlManager.h"

using namespace fly_control;

int main() {
    auto& fc_mgr = MyFlyControlManager::GetInstance();

    nlohmann::json cfg = {
        {"port", "/dev/ttyS1"},
        {"baudrate", 115200},
        {"timeout_ms", 100},
        {"auto_open", true}
    };

    std::string err;
    if (!fc_mgr.Init(cfg, &err)) {
        std::cerr << "初始化失败: " << err << std::endl;
        return 1;
    }

    fc_mgr.SetOnHeartbeat([](const HeartbeatData& hb) {
        printf("心跳: 星数=%d, 电量=%d%%\n", hb.satellite_count, hb.battery_percent);
    });

    if (!fc_mgr.Start(&err)) {
        std::cerr << "启动失败: " << err << std::endl;
        return 1;
    }

    fc_mgr.SendSetSpeed(2000);

    if (fc_mgr.HasHeartbeat()) {
        auto hb = fc_mgr.GetLatestHeartbeat();
        printf("当前飞行状态=%u\n", hb.flight_state);
    }

    fc_mgr.Shutdown();
    return 0;
}
```

### 8.2 直接实例模式（高级用法）

```cpp
#include "MyFlyControl.h"

using namespace fly_control;

int main() {
    MyFlyControl fc;

    // 1. 初始化
    nlohmann::json cfg = {
        {"port", "/dev/ttyS1"},
        {"baudrate", 115200},
        {"timeout_ms", 100},
        {"auto_open", true}
    };

    std::string err;
    if (!fc.Init(cfg, &err)) {
        std::cerr << "初始化失败: " << err << std::endl;
        return 1;
    }

    // 2. 注册心跳回调
    fc.SetOnHeartbeat([](const HeartbeatData& hb) {
        printf("心跳: 星数=%d, 电量=%d%%, 纬度=%.7f\n",
               hb.satellite_count, hb.battery_percent,
               hb.latitude * 1e-7);
    });

    // 3. 注册指令回复回调
    fc.SetOnCommandReply([](const CommandReplyData& reply) {
        printf("指令回复: %s = %s\n",
               GetCommandName(reply.replied_cmd).c_str(),
               reply.result == 0 ? "成功" : "失败");
    });

    // 4. 启动
    fc.Start();

    // 5. 发送指令
    // 设置飞行速度 20.00 m/s
    fc.SendSetSpeed(2000);

    // 设置飞行航线
    std::vector<RoutePoint> route = {
        {1163000000, 396000000, 1000, 1, 2000},  // 点1
        {1163100000, 396100000, 1500, 2, 2500},  // 点2
        {1163200000, 396200000, 2000, 3, 1500},  // 点3
    };
    fc.SendSetRoute(route);

    // 6. 读取最新状态
    if (fc.HasHeartbeat()) {
        auto hb = fc.GetLatestHeartbeat();
        auto faults = FaultBits::FromU16(hb.fault_info);
        if (faults.voltage_too_low) {
            printf("警告: 电压过低!\n");
        }
    }

    // 7. 停止
    fc.Stop();
    return 0;
}
```

### 8.3 仅使用帧层和协议层（无需完整业务层）

```cpp
#include "FlyControlFrame.h"
#include "FlyControlCodec.h"

using namespace fly_control;

// 手动编码一帧
SetSpeedData speed_data;
speed_data.speed = 2000;  // 20.00 m/s
auto frame = EncodeSetSpeed(0x01, speed_data);
// frame 是完整字节序列，可直接通过串口发送

// 手动解析字节流
FrameParser parser;
parser.FeedData(received_bytes);

ParsedFrame pf;
while (parser.PopFrame(pf)) {
    if (pf.valid && pf.frame_type == FRAME_TYPE_HEARTBEAT) {
        HeartbeatData hb;
        if (DecodeHeartbeat(pf.payload, hb)) {
            // 使用 hb 中的字段
        }
    }
}
```

### 8.4 故障信息解析

```cpp
auto hb = fc.GetLatestHeartbeat();
auto faults = FaultBits::FromU16(hb.fault_info);

if (faults.gps_lost)         printf("GPS丢失\n");
if (faults.voltage_too_low)  printf("电压过低\n");
if (faults.parachute_failed) printf("开伞失败\n");
if (faults.link_lost)        printf("链路丢失\n");
if (faults.takeoff_failed)   printf("起飞失败\n");
if (faults.attitude_error)   printf("姿态错误\n");
if (faults.return_timeout)   printf("归航超时\n");
```

---

## 9. CMake 集成

### 模块构建

```cmake
# src/util/my_fly_control/CMakeLists.txt
set(BUILD_MY_FLY_CONTROL ON CACHE BOOL "Build my_fly_control library")

add_library(my_fly_control STATIC ...)
target_link_libraries(my_fly_control PUBLIC
    pthread mylog nlohmann_json::nlohmann_json
    my_serial my_serial_core my_tools)
```

### 主项目集成

`src/CMakeLists.txt` 中：
```cmake
set(BUILD_MY_FLY_CONTROL ON CACHE BOOL "Build my_fly_control library")
add_subdirectory(util/my_fly_control)
target_link_libraries(mylib PUBLIC my_fly_control)
```

根 `CMakeLists.txt` 中：
```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE my_fly_control)
```

测试 `cmake/setup_tests.cmake` 中：
```cmake
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE my_fly_control)
```

---

## 10. 单元测试

### 测试概览

| 测试文件 | 测试套件 | 测试数 | 测试内容 |
|----------|----------|--------|----------|
| `TestFlyControlFrame.cpp` | `FlyControlFrameTest` | 10 | 帧层：校验和、帧组装、拆帧、粘包、半包、脏数据 |
| `TestFlyControlCodec.cpp` | `FlyControlCodecTest` | 13 | 编解码：心跳、各指令编解码回环、故障位、端到端 |
| `TestMyFlyControlManager.cpp` | `MyFlyControlManagerTest` | 7 | 管理器：单例入口、生命周期守卫、Shutdown 重建 |

### 运行测试

```bash
cd build
make fast_cpp_server_Test
./bin/fast_cpp_server_Test --gtest_filter="FlyControl*"
```

### 关键测试场景

1. **帧层**：
   - 校验和计算正确性
   - 帧组装结构完整性（帧头、载荷、校验和、帧尾位置正确）
   - 单帧解析
   - 多帧粘包解析
   - 含脏数据帧解析（自动跳过垃圾字节）
   - 半帧（数据分批到达）
   - 心跳帧（89字节固定长度）
   - 回复帧解析

2. **编解码层**：
   - 心跳帧82字段完整编解码回环
   - 故障位 FaultBits 编解码回环
   - 目的地帧编码后帧解析器解码验证
   - 航线帧变长编解码（3个航线点）
   - 电子围栏变长编解码（4个围栏点）
   - 速度/高度帧编码长度和内容验证
   - 指令回复和云台控制解码
   - 末制导指令编码
   - 端到端心跳帧（构建→帧解析→协议解码）

---

## 11. 线程安全设计

| 资源 | 锁 | 说明 |
|------|------|------|
| 心跳数据 `latest_hb_` | `hb_mutex_` | 接收线程写、查询线程读 |
| 串口发送 | `send_mutex_` | 多个业务线程可能同时发送指令 |
| 回调函数 | `cb_mutex_` | 注册和调用回调的线程可能不同 |
| 帧计数 `cnt_` | `std::atomic` | 无锁自增 |
| 运行标志 `running_` | `std::atomic` | 无锁读写 |

### 线程模型

```
主线程           接收线程               其他业务线程
  │                │                      │
  ├─Init()         │                      │
  ├─Start()───────→│ ReceiveLoop()        │
  │                ├─ serial_.ReadBytes()  │
  │                ├─ parser_.FeedData()   │
  │                ├─ parser_.PopFrame()   │
  │                ├─ HandleFrame()        │
  │                │  └─ 更新 latest_hb_   │
  │                │  └─ 调用回调           │
  ├─GetLatestHb()←─│                      ├─SendSetSpeed()
  │                │                      │  └─serial_.Write()
  ├─Stop()────────→│ (退出循环)            │
```

---

## 12. 已知限制与后续扩展

### 当前限制

1. **无超时重发机制** — 当前版本不会自动重发未收到回复的指令。协议规定2秒超时重发，后续可在业务层添加 `SendWithRetry()` 方法。

2. **无心跳超时检测** — 未实现"长时间未收到心跳则认为飞控离线"的检测逻辑。

3. **串口参数固定** — 不支持运行时动态切换串口或波特率，需要 Stop→Init→Start。

### 后续扩展方向

1. **超时重发** — 增加 `PendingCommand` 队列，发送后等待 `CommandReplyData`，超时自动重发。

2. **心跳超时告警** — 维护上次心跳时间戳，超过阈值触发连接丢失回调。

3. **数据录制/回放** — 添加帧级别的录制功能，保存原始字节流用于离线分析。

4. **协议版本协商** — 支持多版本协议切换（如带目标信息的扩展心跳帧）。

5. **统计计数器** — 收发帧计数、校验失败计数、丢帧计数，用于通信质量监控。

---

## 文件清单

```
src/util/my_fly_control/
├── CMakeLists.txt              # 构建配置
├── FlyControlProtocol.h        # 协议常量、枚举、数据结构定义
├── FlyControlFrame.h           # 帧层头文件
├── FlyControlFrame.cpp         # 帧层实现（拆帧、组帧、校验）
├── FlyControlCodec.h           # 编解码层头文件
├── FlyControlCodec.cpp         # 编解码层实现
├── MyFlyControlManager.h       # 管理层头文件（单例包装）
├── MyFlyControlManager.cpp     # 管理层实现
├── MyFlyControl.h              # 业务层头文件
└── MyFlyControl.cpp            # 业务层实现

test/util/my_fly_control/
├── TestFlyControlFrame.cpp     # 帧层单元测试（10个）
├── TestFlyControlCodec.cpp     # 编解码层单元测试（13个）
└── TestMyFlyControlManager.cpp # 管理层单元测试（7个）
```
