#pragma once

/**
 * @file i_ptz_capability.h
 * @brief 云台控制能力接口（PTZ: Pan-Tilt-Zoom）
 */

#include "i_capability.h"
#include "../../common/pod_models.h"

namespace PodModule {

/**
 * @brief 云台控制能力接口
 * 
 * 提供云台姿态获取、速度控制、角度设定等功能。
 */
class IPtzCapability : virtual public ICapability {
public:
    ~IPtzCapability() override = default;

    /** @brief 获取当前云台姿态 */
    virtual PodResult<PTZPose> getPose() = 0;

    /** @brief 设置云台目标角度 */
    virtual PodResult<void> setPose(const PTZPose& pose) = 0;

    /** @brief 云台速度控制 */
    virtual PodResult<void> controlSpeed(const PTZCommand& cmd) = 0;

    /** @brief 停止云台运动 */
    virtual PodResult<void> stop() = 0;

    /** @brief 云台回中 */
    virtual PodResult<void> goHome() = 0;
};

} // namespace PodModule
