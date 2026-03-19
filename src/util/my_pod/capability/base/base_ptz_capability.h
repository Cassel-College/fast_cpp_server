#pragma once

/**
 * @file base_ptz_capability.h
 * @brief 基础云台控制能力
 */

#include "base_capability.h"
#include "../interface/i_ptz_capability.h"

namespace PodModule {

/**
 * @brief 基础云台控制能力
 */
class BasePtzCapability : public BaseCapability, public IPtzCapability {
public:
    BasePtzCapability();
    ~BasePtzCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- IPtzCapability ---
    PodResult<PTZPose> getPose() override;
    PodResult<void> setPose(const PTZPose& pose) override;
    PodResult<void> controlSpeed(const PTZCommand& cmd) override;
    PodResult<void> stop() override;
    PodResult<void> goHome() override;

protected:
    PTZPose current_pose_;  // 当前云台姿态缓存
};

} // namespace PodModule
