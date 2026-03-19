/**
 * @file dji_laser_capability.cpp
 * @brief 大疆激光测距能力实现
 */

#include "dji_laser_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiLaserCapability::DjiLaserCapability() {
    MYLOG_DEBUG("[大疆能力] DjiLaserCapability 构造");
}

DjiLaserCapability::~DjiLaserCapability() {
    MYLOG_DEBUG("[大疆能力] DjiLaserCapability 析构");
}

PodResult<LaserInfo> DjiLaserCapability::measure() {
    MYLOG_DEBUG("{} 大疆激光测距", logPrefix());
    if (!laser_enabled_) {
        return PodResult<LaserInfo>::fail(PodErrorCode::LASER_MEASURE_FAILED, "大疆激光器未开启");
    }
    // TODO: 通过 Session 发送大疆激光测距指令
    LaserInfo info;
    info.is_valid = true;
    info.distance = 250.5;
    info.latitude = 30.123456;
    info.longitude = 120.654321;
    info.altitude = 100.0;
    last_measurement_ = info;
    return PodResult<LaserInfo>::success(info, "大疆激光测距成功（占位）");
}

PodResult<void> DjiLaserCapability::enableLaser() {
    MYLOG_INFO("{} 开启大疆激光器", logPrefix());
    // TODO: 通过 Session 发送开启指令
    laser_enabled_ = true;
    return PodResult<void>::success("大疆激光器已开启（占位）");
}

PodResult<void> DjiLaserCapability::disableLaser() {
    MYLOG_INFO("{} 关闭大疆激光器", logPrefix());
    // TODO: 通过 Session 发送关闭指令
    laser_enabled_ = false;
    return PodResult<void>::success("大疆激光器已关闭（占位）");
}

} // namespace PodModule
