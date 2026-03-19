#pragma once

/**
 * @file i_laser_capability.h
 * @brief 激光测距能力接口
 */

#include "i_capability.h"
#include "../../common/pod_models.h"

namespace PodModule {

/**
 * @brief 激光测距能力接口
 * 
 * 提供激光测距仪的控制和测量功能。
 */
class ILaserCapability : virtual public ICapability {
public:
    ~ILaserCapability() override = default;

    /** @brief 执行一次激光测距 */
    virtual PodResult<LaserInfo> measure() = 0;

    /** @brief 开启激光器 */
    virtual PodResult<void> enableLaser() = 0;

    /** @brief 关闭激光器 */
    virtual PodResult<void> disableLaser() = 0;

    /** @brief 获取激光器开启状态 */
    virtual bool isLaserEnabled() const = 0;
};

} // namespace PodModule
