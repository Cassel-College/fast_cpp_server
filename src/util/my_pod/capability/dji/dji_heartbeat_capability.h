#pragma once

/**
 * @file dji_heartbeat_capability.h
 * @brief 大疆心跳检测能力
 */

#include "../base/base_heartbeat_capability.h"

namespace PodModule {

class DjiHeartbeatCapability : public BaseHeartbeatCapability {
public:
    DjiHeartbeatCapability();
    ~DjiHeartbeatCapability() override;

    PodResult<bool> sendHeartbeat() override;
};

} // namespace PodModule
