#include "FlyControlCodec.h"
#include "FlyControlFrame.h"
#include "MyLEHelper.h"

// =============================================================================
// 协议编解码层实现
// 所有 encode 函数将结构体字段用 MyLEHelper 写入载荷，然后调用 BuildFrame 组帧
// 所有 decode 函数从载荷中用 MyLEHelper 按序读出字段
// =============================================================================

namespace fly_control {

// ===========================================================================
// FaultBits 实现
// ===========================================================================

FaultBits FaultBits::FromU16(uint16_t val) {
    FaultBits fb;
    fb.link_lost         = (val >> 0)  & 1;
    fb.distance_exceed   = (val >> 1)  & 1;
    fb.attitude_exceed   = (val >> 2)  & 1;
    fb.altitude_too_low  = (val >> 4)  & 1;
    fb.altitude_too_high = (val >> 5)  & 1;
    fb.voltage_too_low   = (val >> 6)  & 1;
    fb.mission_fault     = (val >> 7)  & 1;
    fb.gps_lost          = (val >> 8)  & 1;
    fb.takeoff_failed    = (val >> 9)  & 1;
    fb.attitude_error    = (val >> 10) & 1;
    fb.return_timeout    = (val >> 13) & 1;
    fb.return_dist_error = (val >> 14) & 1;
    fb.parachute_failed  = (val >> 15) & 1;
    return fb;
}

uint16_t FaultBits::ToU16() const {
    uint16_t val = 0;
    if (link_lost)         val |= (1 << 0);
    if (distance_exceed)   val |= (1 << 1);
    if (attitude_exceed)   val |= (1 << 2);
    if (altitude_too_low)  val |= (1 << 4);
    if (altitude_too_high) val |= (1 << 5);
    if (voltage_too_low)   val |= (1 << 6);
    if (mission_fault)     val |= (1 << 7);
    if (gps_lost)          val |= (1 << 8);
    if (takeoff_failed)    val |= (1 << 9);
    if (attitude_error)    val |= (1 << 10);
    if (return_timeout)    val |= (1 << 13);
    if (return_dist_error) val |= (1 << 14);
    if (parachute_failed)  val |= (1 << 15);
    return val;
}

// ===========================================================================
// 解码函数实现
// ===========================================================================

// ---------------------------------------------------------------------------
// 解码心跳帧载荷
// payload 布局（不含帧头/CNT/帧类型，即从飞机ID开始）：
//   飞机ID(1) + 运行模式(1) + 定位星数(1) + lon(4) + lat(4) + alt(2) + ...
//   共 82 字节（89 - 帧头2 - CNT1 - 帧类型1 - 校验1 - 帧尾2 = 82）
// ---------------------------------------------------------------------------
bool DecodeHeartbeat(const std::vector<uint8_t>& payload, HeartbeatData& out) {
    // 心跳载荷长度应为 82 字节
    if (payload.size() < 82) {
        return false;
    }

    size_t off = 0;
    out.aircraft_id     = MyLEHelper::read_uint8(payload, off);
    out.run_mode        = MyLEHelper::read_uint8(payload, off);
    out.satellite_count = MyLEHelper::read_uint8(payload, off);
    out.longitude       = static_cast<int32_t>(MyLEHelper::read_uint32(payload, off));
    out.latitude        = static_cast<int32_t>(MyLEHelper::read_uint32(payload, off));
    out.altitude        = MyLEHelper::read_uint16(payload, off);
    out.relative_alt    = MyLEHelper::read_uint16(payload, off);
    out.airspeed        = MyLEHelper::read_uint16(payload, off);
    out.groundspeed     = MyLEHelper::read_uint16(payload, off);
    out.velocity_x      = MyLEHelper::read_int16(payload, off);
    out.velocity_y      = MyLEHelper::read_int16(payload, off);
    out.velocity_z      = MyLEHelper::read_int16(payload, off);
    out.accel_x         = MyLEHelper::read_int16(payload, off);
    out.accel_y         = MyLEHelper::read_int16(payload, off);
    out.accel_z         = MyLEHelper::read_int16(payload, off);
    out.roll            = MyLEHelper::read_int16(payload, off);
    out.pitch           = MyLEHelper::read_int16(payload, off);
    out.yaw             = MyLEHelper::read_uint16(payload, off);
    out.flight_mode     = MyLEHelper::read_uint8(payload, off);
    out.current_waypoint = MyLEHelper::read_uint16(payload, off);
    out.battery_voltage = MyLEHelper::read_uint16(payload, off);
    out.battery_percent = MyLEHelper::read_uint8(payload, off);
    out.battery_current = MyLEHelper::read_uint16(payload, off);
    out.atm_pressure    = MyLEHelper::read_uint16(payload, off);
    out.utc_timestamp   = MyLEHelper::read_uint64(payload, off);
    out.fault_info      = MyLEHelper::read_uint16(payload, off);
    out.rotation_speed  = MyLEHelper::read_uint16(payload, off);
    out.throttle        = MyLEHelper::read_uint16(payload, off);
    out.target_altitude = MyLEHelper::read_uint16(payload, off);
    out.target_speed    = MyLEHelper::read_uint16(payload, off);
    out.origin_distance = MyLEHelper::read_uint16(payload, off);
    out.origin_heading  = MyLEHelper::read_uint16(payload, off);
    out.target_distance = MyLEHelper::read_uint16(payload, off);
    out.flight_state    = MyLEHelper::read_uint8(payload, off);
    out.altitude_state  = MyLEHelper::read_uint8(payload, off);
    out.state_switch_src = MyLEHelper::read_uint8(payload, off);
    out.flight_time     = MyLEHelper::read_uint16(payload, off);
    out.flight_range    = MyLEHelper::read_uint16(payload, off);
    out.reserved        = MyLEHelper::read_uint32(payload, off);

    return true;
}

// ---------------------------------------------------------------------------
// 解码云台控制帧载荷
// payload: 使能(1) + 俯仰(2) + 偏航(2) = 5 字节
// ---------------------------------------------------------------------------
bool DecodeGimbalControl(const std::vector<uint8_t>& payload, GimbalControlData& out) {
    if (payload.size() < 5) {
        return false;
    }
    size_t off = 0;
    out.enable      = MyLEHelper::read_uint8(payload, off);
    out.pitch_angle = MyLEHelper::read_int16(payload, off);
    out.yaw_angle   = MyLEHelper::read_int16(payload, off);
    return true;
}

// ---------------------------------------------------------------------------
// 解码指令回复帧载荷
// payload: 回复指令类型(1) + 结果(1) = 2 字节
// ---------------------------------------------------------------------------
bool DecodeCommandReply(const std::vector<uint8_t>& payload, CommandReplyData& out) {
    if (payload.size() < 2) {
        return false;
    }
    size_t off = 0;
    out.replied_cmd = MyLEHelper::read_uint8(payload, off);
    out.result      = MyLEHelper::read_uint8(payload, off);
    return true;
}

// ===========================================================================
// 编码函数实现
// ===========================================================================

// ---------------------------------------------------------------------------
// 编码：设置飞行目的地（帧类型 0x02）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetDestination(uint8_t cnt, const SetDestinationData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.longitude));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.latitude));
    MyLEHelper::write_uint16(payload, data.altitude);
    return BuildFrame(cnt, FRAME_TYPE_SET_DESTINATION, payload);
}

// ---------------------------------------------------------------------------
// 编码：设置飞行航线（帧类型 0x10, 变长）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetRoute(uint8_t cnt, const SetRouteData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, static_cast<uint8_t>(data.points.size()));
    for (const auto& pt : data.points) {
        MyLEHelper::write_uint32(payload, static_cast<uint32_t>(pt.longitude));
        MyLEHelper::write_uint32(payload, static_cast<uint32_t>(pt.latitude));
        MyLEHelper::write_uint16(payload, pt.altitude);
        MyLEHelper::write_uint16(payload, pt.index);
        MyLEHelper::write_uint16(payload, pt.speed);
    }
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：设置角度（帧类型 0x10, 指令 0x03）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetAngle(uint8_t cnt, const SetAngleData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_int16(payload, data.pitch);
    MyLEHelper::write_uint16(payload, data.yaw);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：设置速度（帧类型 0x10, 指令 0x04）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetSpeed(uint8_t cnt, const SetSpeedData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint16(payload, data.speed);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：设置高度（帧类型 0x10, 指令 0x05）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetAltitude(uint8_t cnt, const SetAltitudeData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.altitude_type);
    MyLEHelper::write_uint16(payload, data.altitude);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：电源开关（帧类型 0x10, 指令 0x06）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodePowerSwitch(uint8_t cnt, const PowerSwitchData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.command);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：开伞控制（帧类型 0x10, 指令 0x07）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeParachute(uint8_t cnt, const ParachuteData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.parachute_type);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：指令按钮（帧类型 0x10, 指令 0x09）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeButtonCommand(uint8_t cnt, const ButtonCommandData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.button);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：设置原点/返航点（帧类型 0x10, 指令 0x0A）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetOriginReturn(uint8_t cnt, const SetOriginReturnData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.point_type);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.longitude));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.latitude));
    MyLEHelper::write_uint16(payload, data.altitude);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：设置电子围栏（帧类型 0x10, 变长）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSetGeofence(uint8_t cnt, const SetGeofenceData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, static_cast<uint8_t>(data.points.size()));
    for (const auto& pt : data.points) {
        MyLEHelper::write_uint32(payload, static_cast<uint32_t>(pt.longitude));
        MyLEHelper::write_uint32(payload, static_cast<uint32_t>(pt.latitude));
        MyLEHelper::write_uint16(payload, pt.altitude);
        MyLEHelper::write_uint8(payload, pt.index);
    }
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：切换运行模式（帧类型 0x10, 指令 0x0C）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeSwitchMode(uint8_t cnt, const SwitchModeData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.mode);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：末制导指令（帧类型 0x10, 指令 0x0D）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeGuidance(uint8_t cnt, const GuidanceData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint8(payload, data.mode_switch);
    MyLEHelper::write_uint32(payload, data.frame_id);
    MyLEHelper::write_uint32(payload, data.track_id);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_lon));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_lat));
    MyLEHelper::write_uint16(payload, data.target_alt);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：末制导指令新版（帧类型 0x10, 指令 0x0E）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeGuidanceNew(uint8_t cnt, const GuidanceNewData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_int16(payload, data.pitch_los_rate);
    MyLEHelper::write_int16(payload, data.yaw_los_rate);
    MyLEHelper::write_int16(payload, data.pitch_los_angle);
    MyLEHelper::write_int16(payload, data.yaw_los_angle);
    MyLEHelper::write_int16(payload, data.pitch_frame_angle);
    MyLEHelper::write_int16(payload, data.yaw_frame_angle);
    MyLEHelper::write_int16(payload, data.gimbal_pitch_angle);
    MyLEHelper::write_int16(payload, data.gimbal_yaw_angle);
    MyLEHelper::write_int16(payload, data.acc_x);
    MyLEHelper::write_int16(payload, data.acc_y);
    MyLEHelper::write_int16(payload, data.acc_z);
    MyLEHelper::write_int16(payload, data.gyro_x);
    MyLEHelper::write_int16(payload, data.gyro_y);
    MyLEHelper::write_int16(payload, data.gyro_z);
    MyLEHelper::write_uint16(payload, data.target_id);
    MyLEHelper::write_uint16(payload, data.target_type);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_lon));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_lat));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_alt));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.laser_range));
    MyLEHelper::write_uint8(payload, data.status);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：吊舱姿态-角速度模式（帧类型 0x10, 指令 0x0F）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeGimbalAngRate(uint8_t cnt, const GimbalAngRateData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.pitch_los_rate));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.yaw_los_rate));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_lon));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_lat));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.target_alt));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.laser_range));
    MyLEHelper::write_uint8(payload, data.status);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：吊舱姿态-角度模式（帧类型 0x10, 指令 0x10）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeGimbalAngle(uint8_t cnt, const GimbalAngleData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.pitch_frame_angle));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.yaw_frame_angle));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.pitch_ang_rate));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.yaw_ang_rate));
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ---------------------------------------------------------------------------
// 编码：识别目标状态（帧类型 0x10, 指令 0x11）
// ---------------------------------------------------------------------------
std::vector<uint8_t> EncodeTargetState(uint8_t cnt, const TargetStateData& data) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, data.cmd_type);
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.pitch_axis_angle));
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(data.yaw_axis_angle));
    MyLEHelper::write_uint8(payload, data.target_vertical_ratio);
    MyLEHelper::write_uint8(payload, data.target_horizontal_ratio);
    MyLEHelper::write_uint8(payload, data.status);
    return BuildFrame(cnt, FRAME_TYPE_COMMAND, payload);
}

// ===========================================================================
// 辅助函数
// ===========================================================================

std::string GetCommandName(uint8_t cmd_type) {
    switch (cmd_type) {
        case CMD_SET_DESTINATION:   return "设置飞行目的地";
        case CMD_SET_ROUTE:         return "设置飞行航线";
        case CMD_SET_ANGLE:         return "角度控制";
        case CMD_SET_SPEED:         return "速度控制";
        case CMD_SET_ALTITUDE:      return "高度控制";
        case CMD_POWER_SWITCH:      return "载荷电源开关";
        case CMD_PARACHUTE:         return "开伞控制";
        case CMD_BUTTON:            return "指令按钮";
        case CMD_SET_ORIGIN_RETURN: return "设置原点/返航点";
        case CMD_SET_GEOFENCE:      return "设置电子围栏";
        case CMD_SWITCH_MODE:       return "切换运行模式";
        case CMD_GUIDANCE:          return "末制导指令";
        case CMD_GUIDANCE_NEW:      return "末制导指令(新版)";
        case CMD_GIMBAL_ANG_RATE:   return "吊舱姿态-角速度模式";
        case CMD_GIMBAL_ANGLE:      return "吊舱姿态-角度模式";
        case CMD_TARGET_STATE:      return "识别目标状态";
        default:                    return "未知指令";
    }
}

std::string GetFrameTypeName(uint8_t frame_type) {
    switch (frame_type) {
        case FRAME_TYPE_HEARTBEAT:       return "飞控心跳";
        case FRAME_TYPE_SET_DESTINATION: return "设置目的地";
        case FRAME_TYPE_COMMAND:         return "飞行控制指令";
        case FRAME_TYPE_REPLY:           return "指令回复";
        case FRAME_TYPE_GIMBAL_CONTROL:  return "云台控制";
        default:                         return "未知帧类型";
    }
}

} // namespace fly_control
