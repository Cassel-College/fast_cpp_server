#pragma once

/**
 * @file base_laser_capability.h
 * @brief 基础激光测距能力
 */

#include "base_capability.h"
#include "../interface/i_laser_capability.h"
#include <atomic>

namespace PodModule {

/**
 * @brief 基础激光测距能力
 */
class BaseLaserCapability : public BaseCapability, public ILaserCapability {
public:
    BaseLaserCapability();
    ~BaseLaserCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- ILaserCapability ---
    PodResult<LaserInfo> measure() override;
    PodResult<void> enableLaser() override;
    PodResult<void> disableLaser() override;
    bool isLaserEnabled() const override;

protected:
    std::atomic<bool> laser_enabled_{false};  // 激光器是否开启
    LaserInfo last_measurement_;               // 上次测量结果
};

} // namespace PodModule
