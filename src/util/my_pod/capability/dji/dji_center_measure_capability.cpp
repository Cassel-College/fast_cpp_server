/**
 * @file dji_center_measure_capability.cpp
 * @brief 大疆中心点测量能力实现
 */

#include "dji_center_measure_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiCenterMeasureCapability::DjiCenterMeasureCapability() {
    MYLOG_DEBUG("[大疆能力] DjiCenterMeasureCapability 构造");
}

DjiCenterMeasureCapability::~DjiCenterMeasureCapability() {
    MYLOG_DEBUG("[大疆能力] DjiCenterMeasureCapability 析构");
}

PodResult<CenterMeasurementResult> DjiCenterMeasureCapability::measure() {
    MYLOG_DEBUG("{} 大疆中心点测量", logPrefix());
    // TODO: 通过 Session 发送大疆中心测量指令
    CenterMeasurementResult result;
    result.is_valid = true;
    result.distance = 320.0;
    result.latitude = 30.123456;
    result.longitude = 120.654321;
    result.altitude = 85.0;
    result.center_x = 1920.0;
    result.center_y = 1080.0;
    last_result_ = result;
    return PodResult<CenterMeasurementResult>::success(result, "大疆中心点测量成功（占位）");
}

} // namespace PodModule
