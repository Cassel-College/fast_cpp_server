/**
 * @file dji_heartbeat_capability.cpp
 * @brief 大疆心跳检测能力实现
 */

#include "dji_heartbeat_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiHeartbeatCapability::DjiHeartbeatCapability() {
    MYLOG_DEBUG("[大疆能力] DjiHeartbeatCapability 构造");
}

DjiHeartbeatCapability::~DjiHeartbeatCapability() {
    MYLOG_DEBUG("[大疆能力] DjiHeartbeatCapability 析构");
}

PodResult<bool> DjiHeartbeatCapability::sendHeartbeat() {
    MYLOG_DEBUG("{} 发送大疆心跳", logPrefix());
    // TODO: 通过 Session 发送大疆心跳包
    alive_ = true;
    return PodResult<bool>::success(true, "大疆心跳发送成功（占位）");
}

} // namespace PodModule
