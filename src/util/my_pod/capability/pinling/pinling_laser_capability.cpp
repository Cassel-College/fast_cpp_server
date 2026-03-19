/**
 * @file pinling_laser_capability.cpp
 * @brief 品凌激光测距能力实现
 */

#include "pinling_laser_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingLaserCapability::PinlingLaserCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingLaserCapability 构造");
}

PinlingLaserCapability::~PinlingLaserCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingLaserCapability 析构");
}

PodResult<LaserInfo> PinlingLaserCapability::measure() {
    MYLOG_DEBUG("{} 品凌激光测距", logPrefix());
    if (!laser_enabled_) {
        return PodResult<LaserInfo>::fail(PodErrorCode::LASER_MEASURE_FAILED, "品凌激光器未开启");
    }
    // TODO: 通过 Session 发送品凌激光测距指令
    LaserInfo info;
    info.is_valid = true;
    info.distance = 180.0;
    info.latitude = 31.234567;
    info.longitude = 121.456789;
    info.altitude = 50.0;
    last_measurement_ = info;
    return PodResult<LaserInfo>::success(info, "品凌激光测距成功（占位）");
}

PodResult<void> PinlingLaserCapability::enableLaser() {
    MYLOG_INFO("{} 开启品凌激光器", logPrefix());
    laser_enabled_ = true;
    return PodResult<void>::success("品凌激光器已开启（占位）");
}

PodResult<void> PinlingLaserCapability::disableLaser() {
    MYLOG_INFO("{} 关闭品凌激光器", logPrefix());
    laser_enabled_ = false;
    return PodResult<void>::success("品凌激光器已关闭（占位）");
}

} // namespace PodModule
