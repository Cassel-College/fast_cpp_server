/**
 * @file pinling_status_capability.cpp
 * @brief 品凌状态查询能力实现
 */

#include "pinling_status_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingStatusCapability::PinlingStatusCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingStatusCapability 构造");
}

PinlingStatusCapability::~PinlingStatusCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingStatusCapability 析构");
}

PodResult<PodStatus> PinlingStatusCapability::getStatus() {
    MYLOG_DEBUG("{} 获取品凌设备状态", logPrefix());
    // TODO: 通过 Session 向品凌设备查询状态
    cached_status_.state = PodState::CONNECTED;
    cached_status_.temperature = 40.0;
    cached_status_.voltage = 24.0;
    return PodResult<PodStatus>::success(cached_status_, "获取品凌状态成功（占位）");
}

PodResult<void> PinlingStatusCapability::refreshStatus() {
    MYLOG_INFO("{} 刷新品凌设备状态", logPrefix());
    return PodResult<void>::success("品凌状态刷新成功（占位）");
}

} // namespace PodModule
