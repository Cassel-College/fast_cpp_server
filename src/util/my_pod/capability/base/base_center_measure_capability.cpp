/**
 * @file base_center_measure_capability.cpp
 * @brief 基础中心点测量能力实现
 */

#include "base_center_measure_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseCenterMeasureCapability::BaseCenterMeasureCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseCenterMeasureCapability 构造");
}

BaseCenterMeasureCapability::~BaseCenterMeasureCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseCenterMeasureCapability 析构");
    if (continuous_running_) {
        stopContinuousMeasure();
    }
}

CapabilityType BaseCenterMeasureCapability::getType() const {
    return CapabilityType::CENTER_MEASURE;
}

std::string BaseCenterMeasureCapability::getName() const {
    return "中心点测量";
}

PodResult<void> BaseCenterMeasureCapability::initialize() {
    MYLOG_INFO("{} 初始化中心点测量能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<CenterMeasurementResult> BaseCenterMeasureCapability::measure() {
    MYLOG_DEBUG("{} 执行中心点测量", logPrefix());
    // 基础占位：返回模拟数据
    CenterMeasurementResult result;
    result.is_valid = true;
    result.distance = 150.0;
    result.center_x = 960.0;
    result.center_y = 540.0;
    last_result_ = result;
    return PodResult<CenterMeasurementResult>::success(result, "中心点测量成功（基础占位）");
}

PodResult<void> BaseCenterMeasureCapability::startContinuousMeasure(uint32_t interval_ms) {
    MYLOG_INFO("{} 启动连续测量，间隔: {}ms", logPrefix(), interval_ms);
    measure_interval_ms_ = interval_ms;
    continuous_running_ = true;
    // TODO: 启动测量线程/定时器
    return PodResult<void>::success("连续测量已启动（基础占位）");
}

PodResult<void> BaseCenterMeasureCapability::stopContinuousMeasure() {
    MYLOG_INFO("{} 停止连续测量", logPrefix());
    continuous_running_ = false;
    // TODO: 停止测量线程/定时器
    return PodResult<void>::success("连续测量已停止（基础占位）");
}

PodResult<CenterMeasurementResult> BaseCenterMeasureCapability::getLastResult() {
    MYLOG_DEBUG("{} 获取上次测量结果", logPrefix());
    return PodResult<CenterMeasurementResult>::success(last_result_, "获取成功");
}

} // namespace PodModule
