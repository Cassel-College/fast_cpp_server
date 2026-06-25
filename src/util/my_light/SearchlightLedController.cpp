#include "SearchlightLedController.h"

#include "MyLog.h"

namespace SearchlightControl {

SearchlightLedController::SearchlightLedController(SearchlightCommandBuilder& builder)
    : builder_(builder) {}

bool SearchlightLedController::buildOpenLed(std::vector<uint8_t>& frame) {
    MYLOG_INFO("LED 业务：生成开灯指令");
    frame = builder_.makeLedOn();
    return !frame.empty();
}

bool SearchlightLedController::buildCloseLed(std::vector<uint8_t>& frame) {
    MYLOG_INFO("LED 业务：生成关灯指令");
    frame = builder_.makeLedOff();
    return !frame.empty();
}

bool SearchlightLedController::buildLightLevel(int level, std::vector<uint8_t>& frame) {
    if (level < 0 || level > 100) {
        MYLOG_INFO("LED 业务：亮度参数非法，允许范围 0~100");
        return false;
    }
    MYLOG_INFO("LED 业务：生成亮度设置指令，亮度=" + std::to_string(level));
    frame = builder_.makeBrightness(level);
    return !frame.empty();
}

bool SearchlightLedController::buildOpenFlash(std::vector<uint8_t>& frame) {
    MYLOG_INFO("LED 业务：生成开启闪烁指令");
    frame = builder_.makeFlashEnable();
    return !frame.empty();
}

bool SearchlightLedController::buildCloseFlash(std::vector<uint8_t>& frame) {
    MYLOG_INFO("LED 业务：生成关闭闪烁指令");
    frame = builder_.makeFlashDisable();
    return !frame.empty();
}

bool SearchlightLedController::buildFlashHz(int hz, std::vector<uint8_t>& frame) {
    if (hz <= 0 || hz > 50) {
        MYLOG_INFO("LED 业务：闪烁频率非法，允许范围 1~50Hz");
        return false;
    }
    MYLOG_INFO("LED 业务：生成闪烁频率设置指令，频率=" + std::to_string(hz) + "Hz");
    frame = builder_.makeFlashHz(hz);
    return !frame.empty();
}

}  // namespace SearchlightControl
