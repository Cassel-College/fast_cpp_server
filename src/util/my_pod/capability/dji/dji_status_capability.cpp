/**
 * @file dji_status_capability.cpp
 * @brief 大疆状态查询能力实现
 */

#include "dji_status_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiStatusCapability::DjiStatusCapability() {
    MYLOG_DEBUG("[大疆能力] DjiStatusCapability 构造");
}

DjiStatusCapability::~DjiStatusCapability() {
    MYLOG_DEBUG("[大疆能力] DjiStatusCapability 析构");
}

PodResult<PodStatus> DjiStatusCapability::getStatus() {
    MYLOG_DEBUG("{} 获取大疆设备状态", logPrefix());
    // TODO: 通过 Session 向大疆设备查询状态
    cached_status_.state = PodState::CONNECTED;
    cached_status_.temperature = 35.5;
    cached_status_.voltage = 12.0;
    return PodResult<PodStatus>::success(cached_status_, "获取大疆状态成功（占位）");
}

PodResult<void> DjiStatusCapability::refreshStatus() {
    MYLOG_INFO("{} 刷新大疆设备状态", logPrefix());
    // TODO: 实际刷新逻辑
    return PodResult<void>::success("大疆状态刷新成功（占位）");
}

} // namespace PodModule
