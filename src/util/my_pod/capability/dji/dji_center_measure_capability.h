#pragma once

/**
 * @file dji_center_measure_capability.h
 * @brief 大疆中心点测量能力
 */

#include "../base/base_center_measure_capability.h"

namespace PodModule {

class DjiCenterMeasureCapability : public BaseCenterMeasureCapability {
public:
    DjiCenterMeasureCapability();
    ~DjiCenterMeasureCapability() override;

    PodResult<CenterMeasurementResult> measure() override;
};

} // namespace PodModule
