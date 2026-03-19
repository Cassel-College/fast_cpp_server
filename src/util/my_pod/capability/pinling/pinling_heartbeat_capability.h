#pragma once

/**
 * @file pinling_heartbeat_capability.h
 * @brief 品凌心跳检测能力
 */

#include "../base/base_heartbeat_capability.h"

namespace PodModule {

class PinlingHeartbeatCapability : public BaseHeartbeatCapability {
public:
    PinlingHeartbeatCapability();
    ~PinlingHeartbeatCapability() override;

    PodResult<bool> sendHeartbeat() override;
};

} // namespace PodModule
