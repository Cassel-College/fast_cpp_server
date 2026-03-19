#pragma once

/**
 * @file pinling_laser_capability.h
 * @brief 品凌激光测距能力
 */

#include "../base/base_laser_capability.h"

namespace PodModule {

class PinlingLaserCapability : public BaseLaserCapability {
public:
    PinlingLaserCapability();
    ~PinlingLaserCapability() override;

    PodResult<LaserInfo> measure() override;
    PodResult<void> enableLaser() override;
    PodResult<void> disableLaser() override;
};

} // namespace PodModule
