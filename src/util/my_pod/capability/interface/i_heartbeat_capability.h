#pragma once

/**
 * @file i_heartbeat_capability.h
 * @brief 心跳检测能力接口
 */

#include "i_capability.h"

namespace PodModule {

/**
 * @brief 心跳检测能力接口
 * 
 * 提供定期心跳检测，判断设备是否在线。
 */
class IHeartbeatCapability : virtual public ICapability {
public:
    ~IHeartbeatCapability() override = default;

    /** @brief 发送一次心跳并检测设备是否存活 */
    virtual PodResult<bool> sendHeartbeat() = 0;

    /** @brief 启动自动心跳检测 */
    virtual PodResult<void> startHeartbeat(uint32_t interval_ms = 1000) = 0;

    /** @brief 停止自动心跳检测 */
    virtual PodResult<void> stopHeartbeat() = 0;

    /** @brief 获取设备是否在线 */
    virtual bool isAlive() const = 0;
};

} // namespace PodModule
