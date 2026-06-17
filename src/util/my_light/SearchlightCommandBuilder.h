#ifndef SEARCHLIGHT_COMMAND_BUILDER_H
#define SEARCHLIGHT_COMMAND_BUILDER_H

#include <stdint.h>

#include <string>
#include <vector>

namespace SearchlightControl {

// 操作码生成器类：把业务指令转换为协议帧，避免在业务层手写 HEX。
class SearchlightCommandBuilder {
public:
    SearchlightCommandBuilder();

    std::vector<uint8_t> makeLedOn();
    std::vector<uint8_t> makeLedOff();
    std::vector<uint8_t> makeBrightness(int level);
    std::vector<uint8_t> makeFlashEnable();
    std::vector<uint8_t> makeFlashDisable();
    std::vector<uint8_t> makeFlashHz(int hz);
    std::vector<uint8_t> makeServoHome();
    std::vector<uint8_t> makeServoMoveTo(int x_degree, int y_degree);

private:
    std::vector<uint8_t> makeFrame(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> makeLedPayload(uint8_t op, uint8_t value) const;
    static int clamp(int value, int min_value, int max_value);
    static void appendBigEndianInt16(std::vector<uint8_t>& payload, int value);

    uint8_t frame_counter_;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_COMMAND_BUILDER_H
