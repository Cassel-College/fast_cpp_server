/**
 * @file base_pod.cpp
 * @brief 基础吊舱实现
 */

#include "base_pod.h"
#include "MyLog.h"

namespace PodModule {

BasePod::BasePod(const PodInfo& info) : pod_info_(info) {
    MYLOG_INFO("[吊舱] BasePod 构造: id={}, name={}, vendor={}",
               info.pod_id, info.pod_name, podVendorToString(info.vendor));
}

BasePod::~BasePod() {
    MYLOG_INFO("[吊舱] BasePod 析构: {}", pod_info_.pod_id);
}

// ==================== 设备基础信息 ====================

PodInfo BasePod::getPodInfo() const {
    return pod_info_;
}

std::string BasePod::getPodId() const {
    return pod_info_.pod_id;
}

std::string BasePod::getPodName() const {
    return pod_info_.pod_name;
}

PodVendor BasePod::getVendor() const {
    return pod_info_.vendor;
}

// ==================== 连接管理 ====================

PodResult<void> BasePod::connect() {
    if (state_ == PodState::CONNECTED) {
        MYLOG_WARN("[吊舱] {} 已处于连接状态", pod_info_.pod_id);
        return PodResult<void>::fail(PodErrorCode::ALREADY_CONNECTED);
    }

    if (!session_) {
        MYLOG_ERROR("[吊舱] {} 会话未设置，无法连接", pod_info_.pod_id);
        return PodResult<void>::fail(PodErrorCode::SESSION_NOT_INITIALIZED);
    }

    setState(PodState::CONNECTING);
    MYLOG_INFO("[吊舱] {} 正在连接 {}:{}",
               pod_info_.pod_id, pod_info_.ip_address, pod_info_.port);

    auto result = session_->connect(pod_info_.ip_address, pod_info_.port);
    if (result.isSuccess()) {
        setState(PodState::CONNECTED);
        MYLOG_INFO("[吊舱] {} 连接成功", pod_info_.pod_id);
    } else {
        setState(PodState::ERROR);
        MYLOG_ERROR("[吊舱] {} 连接失败: {}", pod_info_.pod_id, result.message);
    }
    return result;
}

PodResult<void> BasePod::disconnect() {
    if (state_ == PodState::DISCONNECTED) {
        MYLOG_WARN("[吊舱] {} 已处于断开状态", pod_info_.pod_id);
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED);
    }

    MYLOG_INFO("[吊舱] {} 正在断开连接", pod_info_.pod_id);
    PodResult<void> result = PodResult<void>::success();

    if (session_ && session_->isConnected()) {
        result = session_->disconnect();
    }

    setState(PodState::DISCONNECTED);
    MYLOG_INFO("[吊舱] {} 已断开连接", pod_info_.pod_id);
    return result;
}

bool BasePod::isConnected() const {
    return state_ == PodState::CONNECTED;
}

PodState BasePod::getState() const {
    return state_;
}

// ==================== 能力管理 ====================

bool BasePod::registerCapability(CapabilityType type, std::shared_ptr<ICapability> capability) {
    return capability_registry_.registerCapability(type, capability);
}

std::shared_ptr<ICapability> BasePod::getCapability(CapabilityType type) const {
    return capability_registry_.getCapability(type);
}

bool BasePod::hasCapability(CapabilityType type) const {
    return capability_registry_.hasCapability(type);
}

std::vector<CapabilityType> BasePod::listCapabilities() const {
    return capability_registry_.listCapabilities();
}

// ==================== Session 管理 ====================

void BasePod::setSession(std::shared_ptr<ISession> session) {
    session_ = session;
    MYLOG_INFO("[吊舱] {} 已设置会话", pod_info_.pod_id);
}

std::shared_ptr<ISession> BasePod::getSession() const {
    return session_;
}

// ==================== 初始化 ====================

PodResult<void> BasePod::initializeCapabilities() {
    MYLOG_INFO("[吊舱] {} 初始化能力装配（基础实现，子类应覆盖）", pod_info_.pod_id);
    // 基础实现不注册任何能力，由厂商 Pod 子类覆盖
    return PodResult<void>::success("能力装配完成（基础）");
}

// ==================== 内部方法 ====================

void BasePod::setState(PodState state) {
    PodState old_state = state_;
    state_ = state;
    MYLOG_DEBUG("[吊舱] {} 状态变更: {} -> {}",
                pod_info_.pod_id, podStateToString(old_state), podStateToString(state));
}

bool BasePod::addCapability(CapabilityType type, std::shared_ptr<ICapability> capability) {
    if (!capability) {
        MYLOG_ERROR("[吊舱] {} 添加能力失败：能力实例为空", pod_info_.pod_id);
        return false;
    }

    // 自动关联 Pod 和 Session
    capability->attachPod(this);
    if (session_) {
        capability->attachSession(session_);
    }

    // 初始化能力
    auto initResult = capability->initialize();
    if (!initResult.isSuccess()) {
        MYLOG_WARN("[吊舱] {} 能力 {} 初始化失败: {}",
                    pod_info_.pod_id, capabilityTypeToString(type), initResult.message);
    }

    // 注册到注册表
    return capability_registry_.registerCapability(type, capability);
}

} // namespace PodModule
