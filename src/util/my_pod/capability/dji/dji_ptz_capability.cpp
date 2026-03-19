/**
 * @file dji_ptz_capability.cpp
 * @brief 大疆云台控制能力实现
 */

#include "dji_ptz_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiPtzCapability::DjiPtzCapability() {
    MYLOG_DEBUG("[大疆能力] DjiPtzCapability 构造");
}

DjiPtzCapability::~DjiPtzCapability() {
    MYLOG_DEBUG("[大疆能力] DjiPtzCapability 析构");
}

PodResult<PTZPose> DjiPtzCapability::getPose() {
    MYLOG_DEBUG("{} 获取大疆云台姿态", logPrefix());
    // TODO: 通过 Session 查询大疆云台角度
    return PodResult<PTZPose>::success(current_pose_, "获取大疆云台姿态成功（占位）");
}

PodResult<void> DjiPtzCapability::setPose(const PTZPose& pose) {
    MYLOG_INFO("{} 设置大疆云台姿态: yaw={}, pitch={}", logPrefix(), pose.yaw, pose.pitch);
    // TODO: 通过 Session 发送大疆云台控制指令
    current_pose_ = pose;
    return PodResult<void>::success("大疆云台姿态设置成功（占位）");
}

PodResult<void> DjiPtzCapability::controlSpeed(const PTZCommand& cmd) {
    MYLOG_DEBUG("{} 大疆云台速度控制", logPrefix());
    // TODO: 通过 Session 发送速度控制指令
    return PodResult<void>::success("大疆速度控制成功（占位）");
}

PodResult<void> DjiPtzCapability::stop() {
    MYLOG_INFO("{} 停止大疆云台", logPrefix());
    // TODO: 发送停止指令
    return PodResult<void>::success("大疆云台已停止（占位）");
}

PodResult<void> DjiPtzCapability::goHome() {
    MYLOG_INFO("{} 大疆云台回中", logPrefix());
    // TODO: 发送回中指令
    current_pose_ = PTZPose{};
    return PodResult<void>::success("大疆云台回中成功（占位）");
}

} // namespace PodModule
