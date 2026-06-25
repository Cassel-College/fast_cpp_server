#include "SearchlightServoController.h"

#include "MyLog.h"

namespace SearchlightControl {

SearchlightServoController::SearchlightServoController(SearchlightCommandBuilder& builder)
    : builder_(builder) {}

bool SearchlightServoController::buildHome(std::vector<uint8_t>& frame) {
    MYLOG_INFO("伺服业务：生成回中指令");
    frame = builder_.makeServoHome();
    return !frame.empty();
}

bool SearchlightServoController::buildMoveTo(int x_degree,
                                             int y_degree,
                                             std::vector<uint8_t>& frame) {
    if (x_degree < -180 || x_degree > 180 || y_degree < -180 || y_degree > 180) {
        MYLOG_INFO("伺服业务：角度参数非法，允许范围 -180~180 度");
        return false;
    }
    MYLOG_INFO("伺服业务：生成绝对角度指令，x=" + std::to_string(x_degree) +
              "，y=" + std::to_string(y_degree));
    frame = builder_.makeServoMoveTo(x_degree, y_degree);
    return !frame.empty();
}

}  // namespace SearchlightControl
