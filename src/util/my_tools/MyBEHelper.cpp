#include "MyBEHelper.h"

#include <stdexcept>

namespace {

void ValidateBitLength(int bit_len) {
	if (bit_len <= 0 || bit_len > 64) {
		throw std::invalid_argument("bit_len must be in range [1, 64]");
	}
}

void EnsureReadable(const std::vector<uint8_t>& buf, size_t offset, size_t bytes_needed) {
	if (offset + bytes_needed > buf.size()) {
		throw std::out_of_range("buffer does not have enough bytes");
	}
}

} // namespace

namespace MyBEHelper {

void write_uint8(std::vector<uint8_t>& buf, uint8_t value) {
	buf.push_back(value);
}

void write_uint16(std::vector<uint8_t>& buf, uint16_t value) {
	buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
	buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

void write_int16(std::vector<uint8_t>& buf, int16_t value) {
	buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
	buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

void write_uint32(std::vector<uint8_t>& buf, uint32_t value) {
	buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
	buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

void write_uint64(std::vector<uint8_t>& buf, uint64_t value) {
	buf.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
	buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
	buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

void write_bits(std::vector<uint8_t>& buf, uint64_t value, int bit_len) {
	ValidateBitLength(bit_len);

	const int byte_len = (bit_len + 7) / 8;
	const size_t start_index = buf.size();
	for (int i = byte_len - 1; i >= 0; --i) {
		buf.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
	}

	const int extra_bits = byte_len * 8 - bit_len;
	if (extra_bits > 0) {
		buf[start_index] &= static_cast<uint8_t>(0xFF >> extra_bits);
	}
}

uint8_t read_uint8(const std::vector<uint8_t>& buf, size_t& offset) {
	EnsureReadable(buf, offset, 1);
	const uint8_t value = buf[offset];
	offset += 1;
	return value;
}

uint16_t read_uint16(const std::vector<uint8_t>& buf, size_t& offset) {
	EnsureReadable(buf, offset, 2);
	const uint16_t value = static_cast<uint16_t>(buf[offset]) << 8 |
						   static_cast<uint16_t>(buf[offset + 1]);
	offset += 2;
	return value;
}

int16_t read_int16(const std::vector<uint8_t>& buf, size_t& offset) {
	return static_cast<int16_t>(read_uint16(buf, offset));
}

uint32_t read_uint32(const std::vector<uint8_t>& buf, size_t& offset) {
	EnsureReadable(buf, offset, 4);
	const uint32_t value = static_cast<uint32_t>(buf[offset]) << 24 |
						   static_cast<uint32_t>(buf[offset + 1]) << 16 |
						   static_cast<uint32_t>(buf[offset + 2]) << 8 |
						   static_cast<uint32_t>(buf[offset + 3]);
	offset += 4;
	return value;
}

uint64_t read_uint64(const std::vector<uint8_t>& buf, size_t& offset) {
	EnsureReadable(buf, offset, 8);
	const uint64_t value = static_cast<uint64_t>(buf[offset]) << 56 |
						   static_cast<uint64_t>(buf[offset + 1]) << 48 |
						   static_cast<uint64_t>(buf[offset + 2]) << 40 |
						   static_cast<uint64_t>(buf[offset + 3]) << 32 |
						   static_cast<uint64_t>(buf[offset + 4]) << 24 |
						   static_cast<uint64_t>(buf[offset + 5]) << 16 |
						   static_cast<uint64_t>(buf[offset + 6]) << 8 |
						   static_cast<uint64_t>(buf[offset + 7]);
	offset += 8;
	return value;
}

uint64_t read_bits(const std::vector<uint8_t>& buf, size_t offset, int bit_len) {
	ValidateBitLength(bit_len);

	const int byte_len = (bit_len + 7) / 8;
	EnsureReadable(buf, offset, static_cast<size_t>(byte_len));

	uint64_t value = 0;
	for (int i = 0; i < byte_len; ++i) {
		value <<= 8;
		value |= buf[offset + static_cast<size_t>(i)];
	}

	if (bit_len < 64) {
		value &= ((1ULL << bit_len) - 1ULL);
	}
	return value;
}

} // namespace MyBEHelper
