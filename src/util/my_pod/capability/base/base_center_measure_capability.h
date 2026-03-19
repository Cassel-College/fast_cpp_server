#pragma once

/**
 * @file base_center_measure_capability.h
 * @brief 基础中心点测量能力
 */

#include "base_capability.h"
#include "../interface/i_center_measure_capability.h"
#include <atomic>

namespace PodModule {

/**
 * @brief 基础中心点测量能力
 * 
 * 结合激光测距和画面信息，测量画面中心点目标的距离和坐标。
 */
class BaseCenterMeasureCapability : public BaseCapability, public ICenterMeasureCapability {
public:
    BaseCenterMeasureCapability();
    ~BaseCenterMeasureCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- ICenterMeasureCapability ---
    PodResult<CenterMeasurementResult> measure() override;
    PodResult<void> startContinuousMeasure(uint32_t interval_ms = 500) override;
    PodResult<void> stopContinuousMeasure() override;
    PodResult<CenterMeasurementResult> getLastResult() override;

protected:
    CenterMeasurementResult last_result_;            // 上次测量结果
    std::atomic<bool> continuous_running_{false};     // 连续测量是否运行中
    uint32_t measure_interval_ms_ = 500;             // 测量间隔
};

} // namespace PodModule
