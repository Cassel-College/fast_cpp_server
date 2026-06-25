# 扬声器框架模块（Speaker Framework）使用手册

## 一、模块概述

扬声器框架模块（`my_audio`）是一套面向嵌入式场景的音频设备硬件抽象层（HAL）框架。采用「基类纯虚接口 + 厂商适配层 + 全局单例管理」的三层架构，支持多厂商多设备的统一管理。

### 架构图

```
┌────────────────────────────────────────────┐
│              MyAudios (单例管理器)            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ NAudio   │  │ NAudio   │  │ XAudio   │  │
│  │ (设备1)  │  │ (设备2)  │  │ (新厂商) │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  │
│       │              │              │        │
│  ┌────┴─────────────┴──────────────┴────┐  │
│  │          BaseAudio (纯虚基类)          │  │
│  └──────────────────────────────────────┘  │
└────────────────────────────────────────────┘
         │                    │
    ┌────┴────┐          ┌───┴─────┐
    │NAudioCore│          │ XAudioSDK│
    │(SDK封装) │          │ (新SDK)  │
    └─────────┘          └─────────┘
```

### 文件结构

```
src/util/my_audio/
├── BaseAudio.h           # 纯虚基类定义（状态枚举、配置结构体、接口）
├── BaseAudio.cpp         # 基类实现（预留）
├── NAudio.h              # NAudio 厂商适配层头文件
├── NAudio.cpp            # NAudio 厂商适配层实现
├── MyAudios.h            # 全局单例管理器头文件
├── MyAudios.cpp          # 全局单例管理器实现
├── CMakeLists.txt        # CMake 构建配置
├── NAudioServerDemo.cpp  # 厂商原始 Demo（不参与编译，仅供参考）
└── NAudioCore/
    ├── NAudioCore.h      # SDK C 接口封装头文件
    └── NAudioCore.cpp    # SDK C 接口封装实现
```

---

## 二、快速开始

### 2.1 编译

模块通过 `BUILD_MY_AUDIO` 标志控制编译，默认开启：

```bash
cd build
cmake .. -DBUILD_MY_AUDIO=ON
make -j$(nproc)
```

### 2.2 配置

在 `config/config.json` 的 `pipeline.executes` 节点中添加音频服务配置：

```json
{
    "9": {
        "model_args": {
            "audio_count": 2,
            "audio_args": {
                "audio_001": {
                    "name": "audio_server_001",
                    "type": "audio_server",
                    "ip": "192.168.1.150",
                    "port": 8890,
                    "device_id": 3445
                },
                "audio_002": {
                    "name": "audio_server_002",
                    "type": "audio_server",
                    "ip": "192.168.1.151",
                    "port": 9998,
                    "device_id": 3446
                }
            }
        },
        "model_name": "audio_server",
        "enable": true
    }
}
```

**配置字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | string | 设备名称（用于日志标识） |
| `type` | string | 设备类型，决定使用哪个厂商适配类 |
| `ip` | string | 设备服务 IP 地址 |
| `port` | int | 设备服务端口 |
| `device_id` | int | SDK 内部设备编号 |
| `default_volume` | int | 默认音量（0-100），可选，默认 50 |

### 2.3 通过 Pipeline 自动启动

配置完成后，Pipeline 会自动调用：

```
Pipeline::LaunchAudioServer(args)
  → MyAudios::GetInstance().Init(args)
  → MyAudios::GetInstance().Start()
```

### 2.4 手动调用示例

```cpp
#include "MyAudios.h"

// 获取所有设备
auto speakers = my_audio::MyAudios::GetInstance().GetAllSpeakers();

// 获取可用设备
auto available = my_audio::MyAudios::GetInstance().GetAvailableSpeakers();

// 在指定设备上播放
my_audio::MyAudios::GetInstance().PlayOnSpeaker("3445", "/path/to/audio.mp3");

// 在所有设备上播放
my_audio::MyAudios::GetInstance().PlayOnAllSpeakers("/path/to/alarm.mp3");

// 设置音量
my_audio::MyAudios::GetInstance().SetVolume("3445", 80);

// 查询状态
auto status = my_audio::MyAudios::GetInstance().Status("3445");

// 获取全部设备信息 JSON
auto info_json = my_audio::MyAudios::GetInstance().StatusAll();
```

---

## 三、如何添加新厂商适配

### 3.1 步骤总览

1. 创建厂商适配类，继承 `BaseAudio`
2. 实现全部纯虚接口
3. 在 `MyAudios::CreateAudioDevice()` 中注册新类型
4. 更新配置文件

### 3.2 详细步骤

#### 步骤 1：创建新厂商适配类

```cpp
// src/util/my_audio/XAudio.h
#pragma once
#include "BaseAudio.h"

namespace my_audio {

class XAudio : public BaseAudio {
public:
    XAudio() = default;
    ~XAudio() override;

    bool Init(const std::string& json_config) override;
    bool Start() override;
    bool Stop() override;
    AudioStatus Status() override;
    AudioConfig Config() override;
    AudioInfo Info() override;
    bool PlayFile(const std::string& path) override;
    bool SetVolume(int volume = 50) override;
    bool CheckSelf() override;
    void AudioLoop() override;

private:
    AudioConfig config_;
    AudioInfo info_;
    // 厂商特有的 SDK 对象...
};

} // namespace my_audio
```

#### 步骤 2：实现所有纯虚接口

参考 `NAudio.cpp` 的实现模式，重点关注：
- `Init()`: 解析 JSON 配置，初始化内部属性
- `Start()`: 启动 SDK 连接/服务
- `Stop()`: 优雅关闭，释放资源
- `AudioLoop()`: 在循环中定期更新状态，检查 `IsStopRequested()` 退出条件
- `PlayFile()`: 调用厂商 SDK 播放接口
- `SetVolume()`: 音量范围限制在 0-100
- `CheckSelf()`: 执行自检逻辑

#### 步骤 3：注册到工厂方法

编辑 `MyAudios.cpp` 中的 `CreateAudioDevice()`：

```cpp
std::shared_ptr<BaseAudio> MyAudios::CreateAudioDevice(const std::string& type) {
    if (type == "audio_server") {
        return std::make_shared<NAudio>();
    }
    // 新增厂商类型
    else if (type == "x_audio") {
        return std::make_shared<XAudio>();
    }
    return nullptr;
}
```

#### 步骤 4：更新配置

在 `config.json` 中使用新的 `type` 值：

```json
{
    "audio_003": {
        "name": "x_audio_001",
        "type": "x_audio",
        "ip": "192.168.1.200",
        "port": 9000,
        "device_id": 5001
    }
}
```

---

## 四、核心接口参考

### 4.1 BaseAudio（纯虚基类）

| 接口 | 返回 | 说明 |
|------|------|------|
| `Init(json_config)` | `bool` | 初始化设备 |
| `Start()` | `bool` | 启动设备服务 |
| `Stop()` | `bool` | 停止设备 |
| `Status()` | `AudioStatus` | 获取当前状态 |
| `Config()` | `AudioConfig` | 获取设备配置 |
| `Info()` | `AudioInfo` | 获取设备信息 |
| `PlayFile(path)` | `bool` | 播放音频文件 |
| `SetVolume(volume)` | `bool` | 设置音量 (0-100) |
| `CheckSelf()` | `bool` | 设备自检 |
| `AudioLoop()` | `void` | 心跳循环（独立线程） |

### 4.2 AudioStatus 枚举

| 值 | 中文 | 说明 |
|----|------|------|
| `Working` | 工作中 | 正在播放/对讲等 |
| `Idle` | 空闲 | 设备就绪 |
| `Offline` | 离线 | 设备不可达 |
| `Error` | 故障 | 设备异常 |

### 4.3 MyAudios（单例管理器）

| 接口 | 返回 | 说明 |
|------|------|------|
| `GetInstance()` | `MyAudios&` | 获取单例 |
| `Init(config)` | `bool` | 初始化所有设备 |
| `Start()` | `bool` | 启动所有设备 |
| `Stop()` | `void` | 停止所有设备 |
| `GetAllSpeakers()` | `map` | 获取全部设备 |
| `GetAvailableSpeakers()` | `vector` | 获取可用设备 |
| `PlayOnSpeaker(id, path)` | `bool` | 在指定设备播放 |
| `PlayOnAllSpeakers(path)` | `int` | 全部设备播放 |
| `SetVolume(id, vol)` | `bool` | 设置指定设备音量 |
| `Status(id)` | `AudioStatus` | 查询设备状态 |
| `Config(id)` | `AudioConfig` | 查询设备配置 |
| `Info(id)` | `AudioInfo` | 查询设备信息 |
| `StatusAll()` | `json` | 全部设备状态 |

---

## 五、日志输出示例

框架启动时的典型日志：

```
[MyAudios] ====== 开始初始化音频设备管理器 ======
[MyAudios] 预期设备数量=2, 实际配置数量=2
[MyAudios] --- 正在创建设备: audio_001 ---
[NAudio] 厂商适配实例已创建
[NAudio] 初始化完成 -> 名称=audio_server_001, ID=3445, IP=192.168.1.150:8890, SDK设备号=3445
[MyAudios] 设备创建成功: ID=3445, 名称=audio_server_001, 类型=audio_server
[MyAudios] ====== 音频设备管理器初始化完成 ======
[MyAudios] ====== 开始启动所有音频设备 ======
[NAudioCore] 正在启动音频服务, IP=192.168.1.150, 端口=8890
[NAudioCore] 音频服务启动成功
[NAudio] 设备 3445 已上线, 在线设备总数=5
[NAudio] AudioLoop 启动, 设备=audio_server_001
[NAudio] 设备状态变更: 离线 -> 空闲, 设备=audio_server_001
```

---

## 六、单元测试

测试文件位于 `test/util/my_audio/TestMyAudio.cpp`，覆盖：

- `MyAudio_AudioConfig`: JSON 序列化/反序列化、部分字段缺失的默认值
- `MyAudio_AudioStatus`: 状态枚举转中文字符串
- `MyAudio_AudioInfo`: 设备信息序列化
- `MyAudio_MyAudios`: 单例唯一性、错误设备查询、播放失败处理

运行测试：

```bash
cd build
cmake .. && make -j$(nproc)
./bin/fast_cpp_server_Test --gtest_filter="MyAudio_*"
```
