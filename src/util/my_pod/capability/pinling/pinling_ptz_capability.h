#pragma once

/**
 * @file pinling_ptz_capability.h
 * @brief 品凌云台控制能力
 */

#include "../base/base_ptz_capability.h"

namespace PodModule {

class PinlingPtzCapability : public BasePtzCapability {
public:
    PinlingPtzCapability();
    ~PinlingPtzCapability() override;

    PodResult<PTZPose> getPose() override;
    PodResult<void> setPose(const PTZPose& pose) override;
    PodResult<void> controlSpeed(const PTZCommand& cmd) override;
    PodResult<void> stop() override;
    PodResult<void> goHome() override;
};

} // namespace PodModule
