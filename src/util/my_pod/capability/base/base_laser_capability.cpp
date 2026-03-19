/**
 * @file base_laser_capability.cpp
 * @brief 基础激光测距能力实现
 */

#include "base_laser_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseLaserCapability::BaseLaserCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseLaserCapability 构造");
}

BaseLaserCapability::~BaseLaserCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseLaserCapability 析构");
}

CapabilityType BaseLaserCapability::getType() const {
    return CapabilityType::LASER;
}

std::string BaseLaserCapability::getName() const {
    return "激光测距";
}

PodResult<void> BaseLaserCapability::initialize() {
    MYLOG_INFO("{} 初始化激光测距能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<LaserInfo> BaseLaserCapability::measure() {
    MYLOG_DEBUG("{} 执行激光测距", logPrefix());
    if (!laser_enabled_) {
        MYLOG_WARN("{} 激光器未开启，无法测距", logPrefix());
        return PodResult<LaserInfo>::fail(PodErrorCode::LASER_MEASURE_FAILED, "激光器未开启");
    }
    // 基础占位：返回模拟数据
    LaserInfo info;
    info.is_valid = true;
    info.distance = 100.0;
    last_measurement_ = info;
    return PodResult<LaserInfo>::success(info, "测距成功（基础占位）");
}

PodResult<void> BaseLaserCapability::enableLaser() {
    MYLOG_INFO("{} 开启激光器", logPrefix());
    laser_enabled_ = true;
    return PodResult<void>::success("激光器已开启（基础占位）");
}

PodResult<void> BaseLaserCapability::disableLaser() {
    MYLOG_INFO("{} 关闭激光器", logPrefix());
    laser_enabled_ = false;
    return PodResult<void>::success("激光器已关闭（基础占位）");
}

bool BaseLaserCapability::isLaserEnabled() const {
    return laser_enabled_;
}

} // namespace PodModule
