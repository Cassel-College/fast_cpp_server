#pragma once

/**
 * @file dji_status_capability.h
 * @brief 大疆状态查询能力
 */

#include "../base/base_status_capability.h"

namespace PodModule {

class DjiStatusCapability : public BaseStatusCapability {
public:
    DjiStatusCapability();
    ~DjiStatusCapability() override;

    PodResult<PodStatus> getStatus() override;
    PodResult<void> refreshStatus() override;
};

} // namespace PodModule
