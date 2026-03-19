/**
 * @file pinling_center_measure_capability.cpp
 * @brief 品凌中心点测量能力实现
 */

#include "pinling_center_measure_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingCenterMeasureCapability::PinlingCenterMeasureCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingCenterMeasureCapability 构造");
}

PinlingCenterMeasureCapability::~PinlingCenterMeasureCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingCenterMeasureCapability 析构");
}

PodResult<CenterMeasurementResult> PinlingCenterMeasureCapability::measure() {
    MYLOG_DEBUG("{} 品凌中心点测量", logPrefix());
    // TODO: 通过 Session 发送品凌中心测量指令
    CenterMeasurementResult result;
    result.is_valid = true;
    result.distance = 200.0;
    result.latitude = 31.234567;
    result.longitude = 121.456789;
    result.altitude = 60.0;
    result.center_x = 960.0;
    result.center_y = 540.0;
    last_result_ = result;
    return PodResult<CenterMeasurementResult>::success(result, "品凌中心点测量成功（占位）");
}

} // namespace PodModule
