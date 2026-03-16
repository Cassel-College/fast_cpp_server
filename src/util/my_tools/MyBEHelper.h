#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// ---------------------------------------------
// 大端编码/解码工具
// ---------------------------------------------
namespace MyBEHelper {

// 写入 uint8_t 大端
void write_uint8(std::vector<uint8_t>& buf, uint8_t value);

// 写入 uint16_t 大端
void write_uint16(std::vector<uint8_t>& buf, uint16_t value);

// 写入 int16_t 大端
void write_int16(std::vector<uint8_t>& buf, int16_t value);

// 写入 uint32_t 大端
void write_uint32(std::vector<uint8_t>& buf, uint32_t value);

// 写入 uint64_t 大端
void write_uint64(std::vector<uint8_t>& buf, uint64_t value);

// 写入任意长度位域（高位在前），位数必须在 [1, 64] 范围内
void write_bits(std::vector<uint8_t>& buf, uint64_t value, int bit_len);

// 读取 uint8_t 大端，并推进 offset
uint8_t read_uint8(const std::vector<uint8_t>& buf, size_t& offset);

// 读取 uint16_t 大端，并推进 offset
uint16_t read_uint16(const std::vector<uint8_t>& buf, size_t& offset);

// 读取 int16_t 大端，并推进 offset
int16_t read_int16(const std::vector<uint8_t>& buf, size_t& offset);

// 读取 uint32_t 大端，并推进 offset
uint32_t read_uint32(const std::vector<uint8_t>& buf, size_t& offset);

// 读取 uint64_t 大端，并推进 offset
uint64_t read_uint64(const std::vector<uint8_t>& buf, size_t& offset);

// 读取任意长度位域（高位在前），不推进 offset
uint64_t read_bits(const std::vector<uint8_t>& buf, size_t offset, int bit_len);

} // namespace MyBEHelper
