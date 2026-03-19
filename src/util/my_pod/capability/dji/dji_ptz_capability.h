#pragma once

/**
 * @file dji_ptz_capability.h
 * @brief 大疆云台控制能力
 */

#include "../base/base_ptz_capability.h"

namespace PodModule {

class DjiPtzCapability : public BasePtzCapability {
public:
    DjiPtzCapability();
    ~DjiPtzCapability() override;

    PodResult<PTZPose> getPose() override;
    PodResult<void> setPose(const PTZPose& pose) override;
    PodResult<void> controlSpeed(const PTZCommand& cmd) override;
    PodResult<void> stop() override;
    PodResult<void> goHome() override;
};

} // namespace PodModule
