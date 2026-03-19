#pragma once

/**
 * @file base_heartbeat_capability.h
 * @brief 基础心跳检测能力
 */

#include "base_capability.h"
#include "../interface/i_heartbeat_capability.h"
#include <atomic>

namespace PodModule {

/**
 * @brief 基础心跳检测能力
 */
class BaseHeartbeatCapability : public BaseCapability, public IHeartbeatCapability {
public:
    BaseHeartbeatCapability();
    ~BaseHeartbeatCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- IHeartbeatCapability ---
    PodResult<bool> sendHeartbeat() override;
    PodResult<void> startHeartbeat(uint32_t interval_ms = 1000) override;
    PodResult<void> stopHeartbeat() override;
    bool isAlive() const override;

protected:
    std::atomic<bool> alive_{false};         // 设备是否存活
    std::atomic<bool> heartbeat_running_{false}; // 心跳任务是否运行中
    uint32_t heartbeat_interval_ms_ = 1000;  // 心跳间隔
};

} // namespace PodModule
