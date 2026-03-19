/**
 * @file base_heartbeat_capability.cpp
 * @brief 基础心跳检测能力实现
 */

#include "base_heartbeat_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseHeartbeatCapability::BaseHeartbeatCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseHeartbeatCapability 构造");
}

BaseHeartbeatCapability::~BaseHeartbeatCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseHeartbeatCapability 析构");
    if (heartbeat_running_) {
        stopHeartbeat();
    }
}

CapabilityType BaseHeartbeatCapability::getType() const {
    return CapabilityType::HEARTBEAT;
}

std::string BaseHeartbeatCapability::getName() const {
    return "心跳检测";
}

PodResult<void> BaseHeartbeatCapability::initialize() {
    MYLOG_INFO("{} 初始化心跳检测能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<bool> BaseHeartbeatCapability::sendHeartbeat() {
    MYLOG_DEBUG("{} 发送心跳", logPrefix());
    // 基础实现：占位，返回存活
    alive_ = true;
    return PodResult<bool>::success(true, "心跳发送成功（基础占位）");
}

PodResult<void> BaseHeartbeatCapability::startHeartbeat(uint32_t interval_ms) {
    MYLOG_INFO("{} 启动心跳检测，间隔: {}ms", logPrefix(), interval_ms);
    heartbeat_interval_ms_ = interval_ms;
    heartbeat_running_ = true;
    // TODO: 启动心跳线程/定时器
    return PodResult<void>::success("心跳检测已启动（基础占位）");
}

PodResult<void> BaseHeartbeatCapability::stopHeartbeat() {
    MYLOG_INFO("{} 停止心跳检测", logPrefix());
    heartbeat_running_ = false;
    // TODO: 停止心跳线程/定时器
    return PodResult<void>::success("心跳检测已停止（基础占位）");
}

bool BaseHeartbeatCapability::isAlive() const {
    return alive_;
}

} // namespace PodModule
