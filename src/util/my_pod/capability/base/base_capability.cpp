/**
 * @file base_capability.cpp
 * @brief 能力基础实现类
 */

#include "base_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseCapability::BaseCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseCapability 构造");
}

BaseCapability::~BaseCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseCapability 析构");
}

CapabilityType BaseCapability::getType() const {
    return CapabilityType::UNKNOWN;
}

std::string BaseCapability::getName() const {
    return "未知能力";
}

PodResult<void> BaseCapability::initialize() {
    MYLOG_INFO("{} 基础初始化", logPrefix());
    if (!pod_) {
        MYLOG_WARN("{} Pod 未关联，跳过初始化", logPrefix());
    }
    if (!session_) {
        MYLOG_WARN("{} Session 未关联，跳过初始化", logPrefix());
    }
    return PodResult<void>::success("能力初始化成功");
}

void BaseCapability::attachPod(IPod* pod) {
    pod_ = pod;
    MYLOG_DEBUG("{} 已关联 Pod", logPrefix());
}

void BaseCapability::attachSession(std::shared_ptr<ISession> session) {
    session_ = session;
    MYLOG_DEBUG("{} 已关联 Session", logPrefix());
}

IPod* BaseCapability::getPod() const {
    return pod_;
}

std::shared_ptr<ISession> BaseCapability::getSession() const {
    return session_;
}

bool BaseCapability::isSessionReady() const {
    return session_ && session_->isConnected();
}

std::string BaseCapability::logPrefix() const {
    return "[吊舱能力][" + getName() + "]";
}

} // namespace PodModule
