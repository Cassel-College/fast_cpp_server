#pragma once

/**
 * @file base_status_capability.h
 * @brief 基础状态查询能力
 */

#include "base_capability.h"
#include "../interface/i_status_capability.h"

namespace PodModule {

/**
 * @brief 基础状态查询能力
 * 
 * 提供状态查询的通用实现，厂商可继承覆盖特定逻辑。
 */
class BaseStatusCapability : public BaseCapability, public IStatusCapability {
public:
    BaseStatusCapability();
    ~BaseStatusCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- IStatusCapability ---
    PodResult<PodStatus> getStatus() override;
    PodResult<void> refreshStatus() override;

protected:
    PodStatus cached_status_;  // 缓存的状态信息
};

} // namespace PodModule
