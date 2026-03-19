#pragma once

/**
 * @file pinling_center_measure_capability.h
 * @brief 品凌中心点测量能力
 */

#include "../base/base_center_measure_capability.h"

namespace PodModule {

class PinlingCenterMeasureCapability : public BaseCenterMeasureCapability {
public:
    PinlingCenterMeasureCapability();
    ~PinlingCenterMeasureCapability() override;

    PodResult<CenterMeasurementResult> measure() override;
};

} // namespace PodModule
