#pragma once

/**
 * @file pinling_status_capability.h
 * @brief 品凌状态查询能力
 */

#include "../base/base_status_capability.h"

namespace PodModule {

class PinlingStatusCapability : public BaseStatusCapability {
public:
    PinlingStatusCapability();
    ~PinlingStatusCapability() override;

    PodResult<PodStatus> getStatus() override;
    PodResult<void> refreshStatus() override;
};

} // namespace PodModule
