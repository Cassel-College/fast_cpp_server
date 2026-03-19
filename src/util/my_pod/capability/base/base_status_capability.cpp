/**
 * @file base_status_capability.cpp
 * @brief 基础状态查询能力实现
 */

#include "base_status_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseStatusCapability::BaseStatusCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseStatusCapability 构造");
}

BaseStatusCapability::~BaseStatusCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseStatusCapability 析构");
}

CapabilityType BaseStatusCapability::getType() const {
    return CapabilityType::STATUS;
}

std::string BaseStatusCapability::getName() const {
    return "状态查询";
}

PodResult<void> BaseStatusCapability::initialize() {
    MYLOG_INFO("{} 初始化状态查询能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<PodStatus> BaseStatusCapability::getStatus() {
    MYLOG_DEBUG("{} 获取设备状态", logPrefix());
    return PodResult<PodStatus>::success(cached_status_, "获取状态成功（基础实现）");
}

PodResult<void> BaseStatusCapability::refreshStatus() {
    MYLOG_DEBUG("{} 刷新状态缓存", logPrefix());
    // 基础实现：占位，厂商覆盖此方法
    return PodResult<void>::success("刷新状态成功（基础占位）");
}

} // namespace PodModule
