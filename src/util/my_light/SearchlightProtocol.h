#ifndef SEARCHLIGHT_PROTOCOL_H
#define SEARCHLIGHT_PROTOCOL_H

#include <stdint.h>

#include <string>
#include <vector>

namespace SearchlightControl {

struct DecodedFrame {
    DecodedFrame();

    uint8_t cmd_id;
    uint8_t length_byte;
    uint8_t body_length;
    uint8_t frame_counter;
    std::vector<uint8_t> payload;
    uint8_t checksum;
};

// 编/解码类：负责帧头、长度位域、帧计数器、大端数据和异或校验。
class ProtocolCodec {
public:
    static const uint8_t kHeader0 = 0x55;
    static const uint8_t kHeader1 = 0xAA;
    static const uint8_t kCmdId = 0xDC;
    static const size_t kMaxPayloadLength = 61;  // 低 6 位长度最大 63，扣除 CMD 和 LEN。

    static std::vector<uint8_t> encode(uint8_t cmd_id,
                                       const std::vector<uint8_t>& payload,
                                       uint8_t frame_counter);

    static bool tryDecode(std::vector<uint8_t>& buffer,
                          DecodedFrame& frame,
                          std::string& error);

    static uint8_t checksum(const std::vector<uint8_t>& frame_without_checksum);
    static std::string toHex(const std::vector<uint8_t>& bytes);
    static bool fromHex(const std::string& hex_text, std::vector<uint8_t>& bytes);

private:
    static std::string byteToHex(uint8_t value);
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_PROTOCOL_H
