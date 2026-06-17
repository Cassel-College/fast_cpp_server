#ifndef SEARCHLIGHT_LED_CONTROLLER_H
#define SEARCHLIGHT_LED_CONTROLLER_H

#include <vector>
#include <stdint.h>

#include "SearchlightCommandBuilder.h"

namespace SearchlightControl {

// LED 业务类：封装开关、亮度、闪烁和频率设置的业务约束。
class SearchlightLedController {
public:
    explicit SearchlightLedController(SearchlightCommandBuilder& builder);

    bool buildOpenLed(std::vector<uint8_t>& frame);
    bool buildCloseLed(std::vector<uint8_t>& frame);
    bool buildLightLevel(int level, std::vector<uint8_t>& frame);
    bool buildOpenFlash(std::vector<uint8_t>& frame);
    bool buildCloseFlash(std::vector<uint8_t>& frame);
    bool buildFlashHz(int hz, std::vector<uint8_t>& frame);

private:
    SearchlightCommandBuilder& builder_;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_LED_CONTROLLER_H
