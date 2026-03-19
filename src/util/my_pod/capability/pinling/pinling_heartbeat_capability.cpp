/**
 * @file pinling_heartbeat_capability.cpp
 * @brief 品凌心跳检测能力实现
 */

#include "pinling_heartbeat_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingHeartbeatCapability::PinlingHeartbeatCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingHeartbeatCapability 构造");
}

PinlingHeartbeatCapability::~PinlingHeartbeatCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingHeartbeatCapability 析构");
}

PodResult<bool> PinlingHeartbeatCapability::sendHeartbeat() {
    MYLOG_DEBUG("{} 发送品凌心跳", logPrefix());
    // TODO: 通过 Session 发送品凌心跳包
    alive_ = true;
    return PodResult<bool>::success(true, "品凌心跳发送成功（占位）");
}

} // namespace PodModule
