#pragma once

/**
 * @file i_center_measure_capability.h
 * @brief 中心点测量能力接口
 * 
 * 测量画面中心位置目标到激光仪的距离及目标坐标。
 */

#include "i_capability.h"
#include "../../common/pod_models.h"

namespace PodModule {

/**
 * @brief 中心点测量能力接口
 * 
 * 结合激光测距和画面信息，测量画面中心点目标的距离和坐标。
 */
class ICenterMeasureCapability : virtual public ICapability {
public:
    ~ICenterMeasureCapability() override = default;

    /** @brief 执行一次中心点测量 */
    virtual PodResult<CenterMeasurementResult> measure() = 0;

    /** @brief 启动连续中心点测量 */
    virtual PodResult<void> startContinuousMeasure(uint32_t interval_ms = 500) = 0;

    /** @brief 停止连续中心点测量 */
    virtual PodResult<void> stopContinuousMeasure() = 0;

    /** @brief 获取最近一次测量结果 */
    virtual PodResult<CenterMeasurementResult> getLastResult() = 0;
};

} // namespace PodModule
