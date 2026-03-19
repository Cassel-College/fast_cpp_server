/**
 * @file base_ptz_capability.cpp
 * @brief 基础云台控制能力实现
 */

#include "base_ptz_capability.h"
#include "MyLog.h"

namespace PodModule {

BasePtzCapability::BasePtzCapability() {
    MYLOG_DEBUG("[吊舱能力] BasePtzCapability 构造");
}

BasePtzCapability::~BasePtzCapability() {
    MYLOG_DEBUG("[吊舱能力] BasePtzCapability 析构");
}

CapabilityType BasePtzCapability::getType() const {
    return CapabilityType::PTZ;
}

std::string BasePtzCapability::getName() const {
    return "云台控制";
}

PodResult<void> BasePtzCapability::initialize() {
    MYLOG_INFO("{} 初始化云台控制能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<PTZPose> BasePtzCapability::getPose() {
    MYLOG_DEBUG("{} 获取云台姿态", logPrefix());
    return PodResult<PTZPose>::success(current_pose_, "获取姿态成功（基础占位）");
}

PodResult<void> BasePtzCapability::setPose(const PTZPose& pose) {
    MYLOG_INFO("{} 设置云台姿态: yaw={}, pitch={}, roll={}, zoom={}",
               logPrefix(), pose.yaw, pose.pitch, pose.roll, pose.zoom);
    current_pose_ = pose;
    return PodResult<void>::success("设置姿态成功（基础占位）");
}

PodResult<void> BasePtzCapability::controlSpeed(const PTZCommand& cmd) {
    MYLOG_DEBUG("{} 云台速度控制: yaw_speed={}, pitch_speed={}, zoom_speed={}",
                logPrefix(), cmd.yaw_speed, cmd.pitch_speed, cmd.zoom_speed);
    return PodResult<void>::success("速度控制成功（基础占位）");
}

PodResult<void> BasePtzCapability::stop() {
    MYLOG_INFO("{} 停止云台运动", logPrefix());
    return PodResult<void>::success("停止成功（基础占位）");
}

PodResult<void> BasePtzCapability::goHome() {
    MYLOG_INFO("{} 云台回中", logPrefix());
    current_pose_ = PTZPose{};
    return PodResult<void>::success("回中成功（基础占位）");
}

} // namespace PodModule
