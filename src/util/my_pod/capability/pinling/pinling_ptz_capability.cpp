/**
 * @file pinling_ptz_capability.cpp
 * @brief 品凌云台控制能力实现
 */

#include "pinling_ptz_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingPtzCapability::PinlingPtzCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingPtzCapability 构造");
}

PinlingPtzCapability::~PinlingPtzCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingPtzCapability 析构");
}

PodResult<PTZPose> PinlingPtzCapability::getPose() {
    MYLOG_DEBUG("{} 获取品凌云台姿态", logPrefix());
    // TODO: 通过 Session 查询品凌云台角度
    return PodResult<PTZPose>::success(current_pose_, "获取品凌云台姿态成功（占位）");
}

PodResult<void> PinlingPtzCapability::setPose(const PTZPose& pose) {
    MYLOG_INFO("{} 设置品凌云台姿态: yaw={}, pitch={}", logPrefix(), pose.yaw, pose.pitch);
    // TODO: 通过 Session 发送品凌云台控制指令
    current_pose_ = pose;
    return PodResult<void>::success("品凌云台姿态设置成功（占位）");
}

PodResult<void> PinlingPtzCapability::controlSpeed(const PTZCommand& cmd) {
    MYLOG_DEBUG("{} 品凌云台速度控制", logPrefix());
    // TODO: 通过 Session 发送速度控制指令
    return PodResult<void>::success("品凌速度控制成功（占位）");
}

PodResult<void> PinlingPtzCapability::stop() {
    MYLOG_INFO("{} 停止品凌云台", logPrefix());
    return PodResult<void>::success("品凌云台已停止（占位）");
}

PodResult<void> PinlingPtzCapability::goHome() {
    MYLOG_INFO("{} 品凌云台回中", logPrefix());
    current_pose_ = PTZPose{};
    return PodResult<void>::success("品凌云台回中成功（占位）");
}

} // namespace PodModule
