#include "SearchlightProtocol.h"

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace SearchlightControl {

const uint8_t ProtocolCodec::kHeader0;
const uint8_t ProtocolCodec::kHeader1;
const uint8_t ProtocolCodec::kCmdId;
const size_t ProtocolCodec::kMaxPayloadLength;

DecodedFrame::DecodedFrame()
    : cmd_id(0),
      length_byte(0),
      body_length(0),
      frame_counter(0),
      checksum(0) {}

std::vector<uint8_t> ProtocolCodec::encode(uint8_t cmd_id,
                                           const std::vector<uint8_t>& payload,
                                           uint8_t frame_counter) {
    std::vector<uint8_t> frame;
    if (payload.size() > kMaxPayloadLength) {
        return frame;
    }

    // 兼容实测帧：长度低 6 位 = CMD_ID + LEN_BYTE + Payload 的字节数。
    const uint8_t body_length = static_cast<uint8_t>(payload.size() + 2);
    const uint8_t length_byte =
        static_cast<uint8_t>((body_length & 0x3F) | ((frame_counter & 0x03) << 6));

    frame.push_back(kHeader0);
    frame.push_back(kHeader1);
    frame.push_back(cmd_id);
    frame.push_back(length_byte);
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(checksum(frame));
    return frame;
}

bool ProtocolCodec::tryDecode(std::vector<uint8_t>& buffer,
                              DecodedFrame& frame,
                              std::string& error) {
    error.clear();

    while (buffer.size() >= 2) {
        if (buffer[0] == kHeader0 && buffer[1] == kHeader1) {
            break;
        }
        buffer.erase(buffer.begin());
    }

    if (buffer.size() < 4) {
        return false;
    }

    if (buffer[2] != kCmdId) {
        error = "收到未知 CMD_ID，已丢弃当前缓冲区，避免帧错位";
        buffer.clear();
        return false;
    }

    const uint8_t length_byte = buffer[3];
    const uint8_t body_length = static_cast<uint8_t>(length_byte & 0x3F);
    const uint8_t frame_counter = static_cast<uint8_t>((length_byte >> 6) & 0x03);

    if (body_length < 2 || body_length > 63) {
        error = "长度字段非法，已清空接收缓冲区";
        buffer.clear();
        return false;
    }

    const size_t payload_length = static_cast<size_t>(body_length - 2);
    const size_t total_length = payload_length + 5;  // Header(2) + CMD + LEN + Payload + CS。

    if (buffer.size() < total_length) {
        return false;
    }

    std::vector<uint8_t> candidate(buffer.begin(), buffer.begin() + total_length);
    const uint8_t expected = checksum(std::vector<uint8_t>(candidate.begin(), candidate.end() - 1));
    const uint8_t actual = candidate.back();
    if (expected != actual) {
        std::ostringstream oss;
        oss << "校验失败，期望 0x" << byteToHex(expected)
            << "，实际 0x" << byteToHex(actual)
            << "，已清空接收缓冲区";
        error = oss.str();
        buffer.clear();
        return false;
    }

    frame.cmd_id = candidate[2];
    frame.length_byte = length_byte;
    frame.body_length = body_length;
    frame.frame_counter = frame_counter;
    frame.payload.assign(candidate.begin() + 4, candidate.end() - 1);
    frame.checksum = actual;

    buffer.erase(buffer.begin(), buffer.begin() + total_length);
    return true;
}

uint8_t ProtocolCodec::checksum(const std::vector<uint8_t>& frame_without_checksum) {
    uint8_t value = 0;
    if (frame_without_checksum.size() <= 3) {
        return value;
    }
    for (size_t i = 3; i < frame_without_checksum.size(); ++i) {
        value ^= frame_without_checksum[i];
    }
    return value;
}

std::string ProtocolCodec::toHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i != 0) {
            oss << ' ';
        }
        oss << byteToHex(bytes[i]);
    }
    return oss.str();
}

bool ProtocolCodec::fromHex(const std::string& hex_text, std::vector<uint8_t>& bytes) {
    bytes.clear();
    std::string compact;
    compact.reserve(hex_text.size());
    for (size_t i = 0; i < hex_text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(hex_text[i]);
        if (std::isxdigit(c)) {
            compact.push_back(static_cast<char>(std::tolower(c)));
        }
    }

    if (compact.size() % 2 != 0) {
        return false;
    }

    for (size_t i = 0; i < compact.size(); i += 2) {
        const std::string item = compact.substr(i, 2);
        char* end = NULL;
        const long value = std::strtol(item.c_str(), &end, 16);
        if (end == NULL || *end != '\0' || value < 0 || value > 255) {
            bytes.clear();
            return false;
        }
        bytes.push_back(static_cast<uint8_t>(value));
    }
    return true;
}

std::string ProtocolCodec::byteToHex(uint8_t value) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(value);
    return oss.str();
}

}  // namespace SearchlightControl
