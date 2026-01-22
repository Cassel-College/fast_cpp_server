# MQTT 模块使用手册 (User Guide)

## 1. 快速开始

### 1.1 编译集成

在您的 CMake 项目中，确保开启 `BUILD_MY_MQTT` 开关：

```cmake
set(BUILD_MY_MQTT ON)
add_subdirectory(src/util/my_mqtt)
target_link_libraries(your_app PRIVATE my_mqtt)
```

### 1.2 配置文件格式

模块统一使用 `nlohmann::json` 进行配置：

```json
{
    "host": "127.0.0.1",
    "port": 1883,
    "keepalive": 60,
    "client_id": "fast_cpp_server_01",
    "clean_session": true,
    "username": "admin",
    "password": "password"
}
```

## 2. 常用操作代码示例

### 2.1 使用单例客户端（推荐）

这是最简单的集成方式，适用于大部分业务场景。

```cpp
#include "fast_mqtt_client/MFSMQTTClient.h"

using namespace fast_mqtt;

// 1. 初始化 (通常在程序启动时)
nlohmann::json cfg = { {"host", "127.0.0.1"}, {"port", 1883} };
MFSMQTTClient::instance().init(cfg);

// 2. 设置消息回调
MFSMQTTClient::instance().set_message_handler([](const std::string& topic, const std::string& payload){
    std::cout << "收到消息 [" << topic << "]: " << payload << std::endl;
});

// 3. 订阅与发布
MFSMQTTClient::instance().subscribe("sensor/data", 1);
MFSMQTTClient::instance().publish("system/status", "online", 1, true);
```

### 2.2 管理多个客户端

如果您需要同时连接到云端和本地 Broker：

```
#include "fast_mqtt_client/MFMQTTManager.h"

MFMQTTManager manager;
manager.add_client("cloud", cloud_cfg);
manager.add_client("local", local_cfg);

manager.publish("cloud", "v1/devices/telemetry", "{\"temp\":25}");
```

### 2.3 启动本地 MQTT 服务

如果您的环境没有安装标准的 Mosquitto 服务，可以使用代码启动：

```cpp
#include "fast_mqtt_server/MFMQTTServer.h"

MFMQTTServer server;
nlohmann::json server_cfg = {
    {"port", 1883},
    {"allow_anonymous", true},
    {"conf_path", "/tmp/mosquitto.conf"}
};

server.init(server_cfg);
server.start(); // 启动后台进程
// ... 业务逻辑 ...
server.stop();  // 退出时关闭
```

## 3. 注意事项

* **线程安全** : `publish` 接口是线程安全的，可以在任意线程调用。
* **单例限制** : `MFSMQTTClient` 在调用 `init` 前不可进行 `publish/subscribe` 操作，否则内部 `client_` 指针为空，操作将直接返回 `false`。
* **环境变量** : `MFMQTTServer` 支持 `${HOME}` 路径解析，方便跨平台部署配置。
