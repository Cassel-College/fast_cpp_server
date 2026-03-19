#pragma once

/**
 * @file i_status_capability.h
 * @brief 状态查询能力接口
 */

#include "i_capability.h"
#include "../../common/pod_models.h"

namespace PodModule {

/**
 * @brief 状态查询能力接口
 * 
 * 提供查询吊舱设备实时状态的能力。
 */
class IStatusCapability : virtual public ICapability {
public:
    ~IStatusCapability() override = default;

    /** @brief 获取设备当前状态 */
    virtual PodResult<PodStatus> getStatus() = 0;

    /** @brief 刷新状态缓存 */
    virtual PodResult<void> refreshStatus() = 0;
};

} // namespace PodModule
