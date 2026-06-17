#ifndef SEARCHLIGHT_SERVO_CONTROLLER_H
#define SEARCHLIGHT_SERVO_CONTROLLER_H

#include <stdint.h>

#include <vector>

#include "SearchlightCommandBuilder.h"

namespace SearchlightControl {

// 角度控制业务类：负责回中和绝对角度控制的参数检查。
class SearchlightServoController {
public:
    explicit SearchlightServoController(SearchlightCommandBuilder& builder);

    bool buildHome(std::vector<uint8_t>& frame);
    bool buildMoveTo(int x_degree, int y_degree, std::vector<uint8_t>& frame);

private:
    SearchlightCommandBuilder& builder_;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_SERVO_CONTROLLER_H
