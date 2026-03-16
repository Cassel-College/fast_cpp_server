# my_tools 大端/小端编解码工具使用手册

## 1. 模块说明

为了统一项目内部对二进制协议的编码与解码方式，my_tools 模块中新增了两组字节序辅助工具：

1. MyLEHelper：小端编码/解码工具。
2. MyBEHelper：大端编码/解码工具。

对应源码文件如下：

1. src/util/my_tools/MyLEHelper.h
2. src/util/my_tools/MyLEHelper.cpp
3. src/util/my_tools/MyBEHelper.h
4. src/util/my_tools/MyBEHelper.cpp

这组工具适合以下场景：

1. 设备私有二进制协议封包。
2. 串口、网络、文件中的定长字段解析。
3. 需要明确指定大端或小端格式的报文读写。
4. 按位长度封装非整字节字段。

## 2. 接口概览

### 2.1 MyLEHelper

MyLEHelper 提供以下接口：

1. write_uint8
2. write_uint16
3. write_int16
4. write_uint32
5. write_uint64
6. write_bits
7. read_uint8
8. read_uint16
9. read_int16
10. read_uint32
11. read_uint64
12. read_bits

### 2.2 MyBEHelper

MyBEHelper 提供与 MyLEHelper 对应的一套大端接口：

1. write_uint8
2. write_uint16
3. write_int16
4. write_uint32
5. write_uint64
6. write_bits
7. read_uint8
8. read_uint16
9. read_int16
10. read_uint32
11. read_uint64
12. read_bits

## 3. 设计约定

### 3.1 关于 offset

除 read_bits 之外，所有读取接口都使用 size_t& offset 作为入参，并在读取成功后自动推进偏移量。

这样做的目的是方便顺序解析二进制缓冲区。

### 3.2 关于 bit_len

write_bits 和 read_bits 的 bit_len 必须满足：

1. bit_len >= 1
2. bit_len <= 64

否则会抛出 std::invalid_argument。

### 3.3 关于越界保护

读取接口在缓冲区数据不足时会抛出 std::out_of_range。

这比“静默读错数据”更安全，适合协议解析阶段及时暴露问题。

## 4. 小端工具示例

### 4.1 写入定长整数

```cpp
#include "MyLEHelper.h"

std::vector<uint8_t> buf;
MyLEHelper::write_uint8(buf, 0x12);
MyLEHelper::write_uint16(buf, 0x3456);
MyLEHelper::write_uint32(buf, 0x789ABCDEu);
```

此时 buf 中的数据顺序为：

```text
12 56 34 DE BC 9A 78
```

### 4.2 顺序读取

```cpp
size_t offset = 0;
uint8_t  a = MyLEHelper::read_uint8(buf, offset);
uint16_t b = MyLEHelper::read_uint16(buf, offset);
uint32_t c = MyLEHelper::read_uint32(buf, offset);
```

读取完成后，offset 会自动前移到下一个字段起始位置。

### 4.3 小端位域示例

```cpp
std::vector<uint8_t> buf;
MyLEHelper::write_bits(buf, 0xABC, 12);

uint64_t value = MyLEHelper::read_bits(buf, 0, 12);
```

对于 0xABC 和 12 位长度，小端字节序结果为：

```text
BC 0A
```

## 5. 大端工具示例

### 5.1 写入定长整数

```cpp
#include "MyBEHelper.h"

std::vector<uint8_t> buf;
MyBEHelper::write_uint8(buf, 0x12);
MyBEHelper::write_uint16(buf, 0x3456);
MyBEHelper::write_uint32(buf, 0x789ABCDEu);
```

此时 buf 中的数据顺序为：

```text
12 34 56 78 9A BC DE
```

### 5.2 顺序读取

```cpp
size_t offset = 0;
uint8_t  a = MyBEHelper::read_uint8(buf, offset);
uint16_t b = MyBEHelper::read_uint16(buf, offset);
uint32_t c = MyBEHelper::read_uint32(buf, offset);
```

### 5.3 大端位域示例

```cpp
std::vector<uint8_t> buf;
MyBEHelper::write_bits(buf, 0xABC, 12);

uint64_t value = MyBEHelper::read_bits(buf, 0, 12);
```

对于 0xABC 和 12 位长度，大端字节序结果为：

```text
0A BC
```

## 6. 典型协议封包示例

假设有如下报文结构：

1. 帧头：1 字节
2. 长度：2 字节，大端
3. 命令字：2 字节，大端
4. 数据：4 字节，小端

可按如下方式封包：

```cpp
std::vector<uint8_t> packet;

MyBEHelper::write_uint8(packet, 0xAA);
MyBEHelper::write_uint16(packet, 6);
MyBEHelper::write_uint16(packet, 0x1001);
MyLEHelper::write_uint32(packet, 0x12345678);
```

这说明大端和小端工具可以混合使用，用于适配真实协议格式。

## 7. 异常处理建议

推荐在协议解析代码外层做统一异常保护，例如：

```cpp
try {
    size_t offset = 0;
    uint16_t len = MyBEHelper::read_uint16(buf, offset);
    uint32_t payload = MyLEHelper::read_uint32(buf, offset);
} catch (const std::exception& ex) {
    // 记录日志，返回协议错误
}
```

## 8. 单元测试覆盖情况

当前新增测试文件：

1. test/util/my_tools/TestEndianHelpers.cpp

已覆盖内容包括：

1. 小端 uint8/uint16/int16/uint32/uint64 的写入与读取。
2. 大端 uint8/uint16/int16/uint32/uint64 的写入与读取。
3. 非整字节位域的大端与小端 round-trip。
4. 缓冲区不足时抛出越界异常。
5. bit_len 非法时抛出参数异常。

## 9. 注意事项

1. read_bits 不会推进 offset，如果需要连续解析位域，请由调用方自行管理偏移。
2. 这组工具当前只覆盖 ll.txt 中列出的基础整数类型与位域，不包含 float、double 或自定义结构体序列化。
3. 如果业务协议中存在符号扩展、BCD 编码、CRC、变长字段等需求，建议在此基础上继续扩展专用 helper。

## 10. 总结

MyLEHelper 和 MyBEHelper 的目标不是做“大而全”的序列化框架，而是为项目中的底层协议开发提供一组简单、明确、可测试的基础编解码积木。

当协议字段确定、字节序明确时，优先使用这组 helper 可以显著减少手写移位与越界错误。