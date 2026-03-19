#pragma once

/**
 * @file dji_laser_capability.h
 * @brief 大疆激光测距能力
 */

#include "../base/base_laser_capability.h"

namespace PodModule {

class DjiLaserCapability : public BaseLaserCapability {
public:
    DjiLaserCapability();
    ~DjiLaserCapability() override;

    PodResult<LaserInfo> measure() override;
    PodResult<void> enableLaser() override;
    PodResult<void> disableLaser() override;
};

} // namespace PodModule
