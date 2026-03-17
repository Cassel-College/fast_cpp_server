# my_serial 设计说明与使用手册

## 1. 模块定位

my_serial 是 fast_cpp_server 中面向业务层的串口工具模块。

它的职责不是直接替代第三方 serial 库，而是在 my_serial_core 之上提供一层更适合本项目使用的封装，统一以下能力：

1. 使用 JSON 作为初始化入参，降低业务接入门槛。
2. 对串口打开、关闭、收发和状态获取提供稳定接口。
3. 提供自检能力，便于运行期检查串口状态是否正常。
4. 提供自测能力，便于开发和测试阶段验证串口链路读写行为。

当前实现文件位于：

1. src/util/my_serial/MySerial.h
2. src/util/my_serial/MySerial.cpp
3. src/util/my_serial/CMakeLists.txt

底层第三方串口核心库位于：

1. cmake/deps/setup_serial.cmake
2. external/serial

其中，setup_serial.cmake 不再调用 external/serial 自带的 CMake，而是直接将所需源码编译为 my_serial_core 静态库，供 my_serial 模块依赖。

## 2. 设计目标

my_serial 的设计目标如下：

### 2.1 统一配置入口

业务代码只需要提供 JSON 配置，即可完成串口对象初始化，无需再直接处理 serial::Serial 的各类枚举和超时细节。

### 2.2 降低接入复杂度

上层模块可以直接调用：

1. Init
2. Open
3. Close
4. Read
5. Write
6. SelfCheck
7. SelfTestLoopback

避免每个业务点重复写一套串口初始化和异常处理代码。

### 2.3 便于运行期诊断

串口问题经常不是“能不能编过”，而是“运行时是否打开成功、当前是否可读、配置是否生效、目标端口是否存在”。

因此 my_serial 提供：

1. GetSnapshot
2. GetSnapshotJson
3. ListAvailablePorts
4. SelfCheck

这些接口可用于日志、健康检查、调试页面和远程诊断。

### 2.4 便于自动化测试

模块内置 loopback 自测接口，同时测试代码使用 pseudo-terminal 模拟串口，避免必须依赖真实硬件设备。

## 3. 模块结构

当前 my_serial 模块采用单类封装：

```text
my_serial
└── MySerial
    ├── Init                // JSON 初始化
    ├── Open/Close          // 串口生命周期控制
    ├── Write/Read          // 字符串或字节流收发
    ├── ReadLine            // 按行读取
    ├── Available           // 查询可读字节数
    ├── ListAvailablePorts  // 枚举串口
    ├── GetSnapshot         // 获取快照
    ├── SelfCheck           // 串口自检
    └── SelfTestLoopback    // 串口回环自测
```

### 3.1 分层关系

```text
业务模块
    ↓
my_serial::MySerial
    ↓
my_serial_core
    ↓
external/serial
    ↓
操作系统串口设备
```

### 3.2 依赖关系

my_serial 当前依赖：

1. my_serial_core
2. mylog
3. nlohmann_json::nlohmann_json
4. pthread

Linux 下 my_serial_core 还会链接：

1. rt
2. pthread

## 4. 核心数据结构

### 4.1 SerialInitOptions

SerialInitOptions 是内部配置结构体，用于承接 JSON 配置并转换为 serial::Serial 需要的参数。

主要字段如下：

1. port：串口设备路径，例如 /dev/ttyUSB0。
2. baudrate：波特率，例如 9600、115200。
3. timeout_ms：简化超时时间，默认 1000ms。
4. inter_byte_timeout_ms：字节间超时。
5. read_timeout_constant_ms：读超时常量。
6. read_timeout_multiplier_ms：读超时倍率。
7. write_timeout_constant_ms：写超时常量。
8. write_timeout_multiplier_ms：写超时倍率。
9. bytesize：数据位，默认 eightbits。
10. parity：校验位，默认 none。
11. stopbits：停止位，默认 one。
12. flowcontrol：流控方式，默认 none。
13. auto_open：初始化后是否自动打开串口，默认 true。

### 4.2 SerialPortSnapshot

用于表示当前串口实例的运行状态。

主要字段如下：

1. initialized：是否完成 Init。
2. open：串口当前是否已经打开。
3. port：当前配置的串口路径。
4. baudrate：当前波特率。
5. timeout_ms：当前基础超时。
6. available_bytes：当前缓冲区中可读取的字节数。
7. bytesize：当前数据位配置。
8. parity：当前校验位配置。
9. stopbits：当前停止位配置。
10. flowcontrol：当前流控配置。
11. last_error：最近一次错误信息。

### 4.3 SerialSelfCheckResult

用于表示自检或自测结果。

字段如下：

1. success：是否成功。
2. summary：摘要信息。
3. detail：详细 JSON 信息。

## 5. 接口设计说明

### 5.1 Init

原型：

```cpp
bool Init(const nlohmann::json& cfg, std::string* err = nullptr);
```

作用：

1. 解析 JSON 配置。
2. 构造 serial::Serial 对象。
3. 设置端口、波特率、超时、数据位、校验位、停止位和流控。
4. 如果 auto_open 为 true，则直接打开串口。

行为约束：

1. cfg 必须是对象类型。
2. port 必须存在且不能为空。
3. 若配置非法，会返回 false，并通过 err 返回错误原因。

### 5.2 Open

原型：

```cpp
bool Open(std::string* err = nullptr);
```

作用：

1. 在已经 Init 的前提下打开串口。
2. 如果已经打开，则不会重复打开。

### 5.3 Close

原型：

```cpp
void Close();
```

作用：

1. 关闭串口。
2. 一般在对象析构或业务退出时调用。

### 5.4 Write

支持两种写入方式：

```cpp
size_t Write(const std::string& data, std::string* err = nullptr);
size_t Write(const std::vector<uint8_t>& data, std::string* err = nullptr);
```

作用：

1. 字符串写入，适合 AT 命令、文本协议。
2. 字节流写入，适合二进制协议。

返回值：

返回实际写入字节数。

### 5.5 Read / ReadBytes / ReadLine

读取接口如下：

```cpp
std::string Read(size_t size, std::string* err = nullptr);
std::vector<uint8_t> ReadBytes(size_t size, std::string* err = nullptr);
std::string ReadLine(size_t max_size = 65536, const std::string& eol = "\n", std::string* err = nullptr);
```

适用场景：

1. Read：读取固定长度文本内容。
2. ReadBytes：读取固定长度二进制数据。
3. ReadLine：读取行协议数据，如以 \n 结尾的串口文本协议。

### 5.6 Available

原型：

```cpp
size_t Available(std::string* err = nullptr) const;
```

作用：

返回当前串口缓冲区中可立即读取的字节数。

### 5.7 ListAvailablePorts

原型：

```cpp
std::vector<nlohmann::json> ListAvailablePorts() const;
```

作用：

枚举当前系统中可识别的串口设备。

返回 JSON 数组项格式：

```json
{
  "port": "/dev/ttyUSB0",
  "description": "USB Serial Device",
  "hardware_id": "USB VID:PID=1A86:7523"
}
```

### 5.8 GetSnapshot / GetSnapshotJson

作用：

1. 输出结构化状态快照。
2. 便于日志、Web 页面或健康检查模块直接消费。

### 5.9 SelfCheck

原型：

```cpp
SerialSelfCheckResult SelfCheck() const;
```

自检内容包括：

1. 是否已经初始化。
2. 串口是否处于打开状态。
3. 当前配置的 port 是否在可枚举串口列表中。
4. 当前快照信息是否可正常导出。

注意：

SelfCheck 更偏向“状态检查”，它不会主动向设备发送测试数据。

### 5.10 SelfTestLoopback

原型：

```cpp
SerialSelfCheckResult SelfTestLoopback(const std::string& payload, size_t read_size = 0);
```

作用：

1. 向串口写入 payload。
2. 从串口读取指定长度数据。
3. 对比收发内容是否一致。

适用场景：

1. 开发环境下验证读写链路。
2. 回环串口或伪终端设备上的自动化测试。
3. 现场调试时快速验证串口是否具备收发能力。

注意：

1. SelfTestLoopback 需要对端具备回显或回环能力。
2. 如果接的是普通外设，但对端不会回显，则该接口不能直接判断业务协议是否正确。

## 6. JSON 配置格式

### 6.1 最小配置

```json
{
  "port": "/dev/ttyUSB0"
}
```

### 6.2 推荐配置

```json
{
  "port": "/dev/ttyUSB0",
  "baudrate": 115200,
  "timeout_ms": 500,
  "bytesize": "eightbits",
  "parity": "none",
  "stopbits": "one",
  "flowcontrol": "none",
  "auto_open": true
}
```

### 6.3 完整配置

```json
{
  "port": "/dev/ttyUSB0",
  "baudrate": 115200,
  "timeout_ms": 500,
  "inter_byte_timeout_ms": 0,
  "read_timeout_constant_ms": 500,
  "read_timeout_multiplier_ms": 0,
  "write_timeout_constant_ms": 500,
  "write_timeout_multiplier_ms": 0,
  "bytesize": "eightbits",
  "parity": "none",
  "stopbits": "one",
  "flowcontrol": "none",
  "auto_open": true
}
```

### 6.4 可选字符串枚举值

> 兼容说明：除 `port` / `baudrate` / `bytesize` / `stopbits` / `flowcontrol` 外，当前也兼容 `device` / `baud_rate` / `data_bits` / `stop_bits` / `flow_control` 这组字段，便于直接接入现有设备配置。

bytesize 支持：

1. fivebits
2. sixbits
3. sevenbits
4. eightbits
5. 5
6. 6
7. 7
8. 8

parity 支持：

1. none
2. odd
3. even
4. mark
5. space

stopbits 支持：

1. one
2. one_point_five
3. two
4. 1
5. 1.5
6. 2

flowcontrol 支持：

1. none
2. software
3. hardware

## 7. 典型使用方式

### 7.1 基础初始化与收发示例

```cpp
#include "MySerial.h"

my_serial::MySerial serial_tool;
std::string err;

nlohmann::json cfg = {
    {"port", "/dev/ttyUSB0"},
    {"baudrate", 115200},
    {"timeout_ms", 500},
    {"auto_open", true}
};

if (!serial_tool.Init(cfg, &err)) {
    std::cerr << "Init failed: " << err << std::endl;
    return;
}

serial_tool.Write("AT\r\n", &err);
std::string response = serial_tool.ReadLine(1024, "\n", &err);

if (!err.empty()) {
    std::cerr << "serial io failed: " << err << std::endl;
}
```

### 7.2 获取状态快照

```cpp
nlohmann::json snapshot = serial_tool.GetSnapshotJson();
std::cout << snapshot.dump(2) << std::endl;
```

### 7.3 执行自检

```cpp
auto check = serial_tool.SelfCheck();
if (!check.success) {
    std::cerr << "self check failed: " << check.summary << std::endl;
}
std::cout << check.detail.dump(2) << std::endl;
```

### 7.4 执行回环自测

```cpp
auto test_result = serial_tool.SelfTestLoopback("hello-loop");
if (!test_result.success) {
    std::cerr << "loopback self test failed: " << test_result.summary << std::endl;
}
```

## 8. CMake 集成方式

### 8.1 模块构建开关

在 src/CMakeLists.txt 中通过以下开关控制：

```cmake
set(BUILD_MY_SERIAL ON CACHE BOOL "Build my_serial library")
```

### 8.2 子目录接入

```cmake
add_subdirectory(util/my_serial)
```

### 8.3 依赖链接

my_serial 模块内部会链接：

```cmake
target_link_libraries(my_serial PUBLIC pthread)
target_link_libraries(my_serial PUBLIC mylog)
target_link_libraries(my_serial PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(my_serial PUBLIC my_serial_core)
```

主程序和 mylib 当前也已经接入 my_serial。

## 9. 自测与测试策略

### 9.1 单元测试文件

测试文件位于：

1. test/util/my_serial/TestMySerial.cpp

### 9.2 当前测试覆盖点

当前已覆盖以下场景：

1. 缺少 port 时初始化失败。
2. 串口枚举接口返回 JSON 结构正确。
3. 使用 pseudo-terminal 模拟串口，验证 Init、Open、Write、Read、SelfCheck。
4. 使用 pseudo-terminal 模拟回环场景，验证 SelfTestLoopback。

### 9.3 测试实现方式

在 Linux 或类 Unix 环境下，测试使用 openpty 创建一对主从伪终端：

1. 从设备路径作为 my_serial 的串口配置。
2. 主设备 fd 作为测试侧模拟对端。
3. 通过主设备写入或读取，验证 MySerial 的真实读写行为。

这种方式的优点是：

1. 不依赖物理串口硬件。
2. 可以进入 CI 或开发机自动化测试。
3. 能覆盖真实串口 API 调用路径，而不是只做假对象测试。

## 10. 稳定性设计说明

### 10.1 线程安全

MySerial 内部使用 mutex 保护状态和串口对象，避免以下问题：

1. 一个线程关闭串口，另一个线程同时读写。
2. 快照查询和收发同时进行时产生竞态。
3. 错误信息与状态信息写入不一致。

### 10.2 初始化状态保护

所有读写和状态接口都以 initialized 和 serial_ 是否有效为前提，避免未初始化即调用串口操作。

### 10.3 错误信息回传

各接口在失败时支持通过 err 输出错误信息，并同时缓存 last_error，便于后续通过快照和自检接口查看最近失败原因。

### 10.4 底层能力与项目接口解耦

my_serial_core 负责底层串口访问，my_serial 负责项目语义层接口。这样做有两个好处：

1. 未来如果更换底层串口库，只需要重构 my_serial_core 接入和 my_serial 的适配层。
2. 上层业务只依赖 my_serial，不需要感知第三方库 API。

## 11. 当前限制

当前版本的 my_serial 仍有以下边界：

1. 仅提供单串口对象封装，还没有多实例统一管理器。
2. SelfCheck 偏向状态检查，不会主动验证业务协议内容。
3. SelfTestLoopback 依赖对端具备回显或回环能力。
4. 当前没有单独的后台读线程和回调机制，如需事件驱动式串口接收，需要在后续版本中扩展。

## 12. 后续扩展建议

后续可以按以下方向增强：

1. 增加 MySerialManager，统一管理多个串口实例。
2. 增加异步接收线程和消息回调接口。
3. 增加基于 JSON 的协议级自检动作。
4. 接入系统健康模块，对串口状态进行周期上报。
5. 在 API 层提供串口调试和回环测试接口。

## 13. 总结

my_serial 的核心价值在于：

1. 屏蔽第三方 serial 细节。
2. 统一本项目串口配置与使用方式。
3. 提供运行期诊断、自检和自动化自测能力。
4. 为后续串口设备、串口协议和现场诊断能力提供可扩展基础。

对于本项目而言，my_serial 不是一个简单的第三方库透传，而是串口能力的项目级标准接入层。