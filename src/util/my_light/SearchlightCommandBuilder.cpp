#include "SearchlightCommandBuilder.h"

#include <vector>

#include "SearchlightProtocol.h"

namespace SearchlightControl {

SearchlightCommandBuilder::SearchlightCommandBuilder() : frame_counter_(0) {}

std::vector<uint8_t> SearchlightCommandBuilder::makeLedOn() {
    return makeFrame(makeLedPayload(0x01, 0x00));
}

std::vector<uint8_t> SearchlightCommandBuilder::makeLedOff() {
    return makeFrame(makeLedPayload(0x02, 0x00));
}

std::vector<uint8_t> SearchlightCommandBuilder::makeBrightness(int level) {
    const int safe_level = clamp(level, 0, 100);
    return makeFrame(makeLedPayload(0x0A, static_cast<uint8_t>(safe_level)));
}

std::vector<uint8_t> SearchlightCommandBuilder::makeFlashEnable() {
    return makeFrame(makeLedPayload(0x09, 0x00));
}

std::vector<uint8_t> SearchlightCommandBuilder::makeFlashDisable() {
    // 关闭闪烁使用 0x00 操作码；校验由协议层重新计算。
    return makeFrame(makeLedPayload(0x00, 0x00));
}

std::vector<uint8_t> SearchlightCommandBuilder::makeFlashHz(int hz) {
    const int safe_hz = clamp(hz, 1, 50);
    return makeFrame(makeLedPayload(0x0B, static_cast<uint8_t>(safe_hz)));
}

std::vector<uint8_t> SearchlightCommandBuilder::makeServoHome() {
    std::vector<uint8_t> payload;
    payload.push_back(0x30);
    payload.push_back(0x04);
    while (payload.size() < 15) {
        payload.push_back(0x00);
    }
    return makeFrame(payload);
}

std::vector<uint8_t> SearchlightCommandBuilder::makeServoMoveTo(int x_degree, int y_degree) {
    // 协议样例显示 30 度 -> 0x1554，即 1 度约等于 182 个伺服单位。
    const int unit_per_degree = 182;
    const int x_raw = clamp(x_degree, -180, 180) * unit_per_degree;
    const int y_raw = -clamp(y_degree, -180, 180) * unit_per_degree;

    std::vector<uint8_t> payload;
    payload.push_back(0x30);
    payload.push_back(0x0B);
    appendBigEndianInt16(payload, x_raw);
    appendBigEndianInt16(payload, y_raw);
    while (payload.size() < 15) {
        payload.push_back(0x00);
    }
    return makeFrame(payload);
}

std::vector<uint8_t> SearchlightCommandBuilder::makeFrame(const std::vector<uint8_t>& payload) {
    const std::vector<uint8_t> frame =
        ProtocolCodec::encode(ProtocolCodec::kCmdId, payload, frame_counter_);
    frame_counter_ = static_cast<uint8_t>((frame_counter_ + 1) & 0x03);
    return frame;
}

std::vector<uint8_t> SearchlightCommandBuilder::makeLedPayload(uint8_t op, uint8_t value) const {
    std::vector<uint8_t> payload;
    payload.push_back(0x2C);
    payload.push_back(0x74);
    payload.push_back(op);
    payload.push_back(value);
    return payload;
}

int SearchlightCommandBuilder::clamp(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

void SearchlightCommandBuilder::appendBigEndianInt16(std::vector<uint8_t>& payload, int value) {
    const int safe = clamp(value, -32768, 32767);
    const uint16_t raw = static_cast<uint16_t>(static_cast<int16_t>(safe));
    payload.push_back(static_cast<uint8_t>((raw >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(raw & 0xFF));
}

}  // namespace SearchlightControl
