#pragma once

#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
// 飞控通信协议定义
// 参考文档：域控飞控通信协议v1.0_20260313.md
// 本文件定义帧结构常量、帧类型、指令类型、枚举值和所有协议数据结构
// =============================================================================

namespace fly_control {

// ---------------------------------------------------------------------------
// 帧结构常量
// ---------------------------------------------------------------------------
constexpr uint8_t  FRAME_HEADER_0   = 0xEB;   // 帧头第一字节
constexpr uint8_t  FRAME_HEADER_1   = 0x90;   // 帧头第二字节
constexpr uint8_t  FRAME_TAIL_0     = 0xED;   // 帧尾第一字节
constexpr uint8_t  FRAME_TAIL_1     = 0xDE;   // 帧尾第二字节
constexpr size_t   FRAME_HEADER_LEN = 2;      // 帧头长度
constexpr size_t   FRAME_TAIL_LEN   = 2;      // 帧尾长度
constexpr size_t   FRAME_CHECKSUM_LEN = 1;    // 校验和长度

// ---------------------------------------------------------------------------
// 帧类型定义
// ---------------------------------------------------------------------------
constexpr uint8_t  FRAME_TYPE_HEARTBEAT        = 0x01;  // 飞控状态心跳帧
constexpr uint8_t  FRAME_TYPE_SET_DESTINATION  = 0x02;  // 设置飞行目的地
constexpr uint8_t  FRAME_TYPE_COMMAND          = 0x10;  // 飞行控制指令帧
constexpr uint8_t  FRAME_TYPE_REPLY            = 0x20;  // 飞行控制指令回复帧
constexpr uint8_t  FRAME_TYPE_GIMBAL_CONTROL   = 0x30;  // 飞控控制云台转动指令

// ---------------------------------------------------------------------------
// 指令类型定义（帧类型 0x10 内的子指令）
// ---------------------------------------------------------------------------
constexpr uint8_t  CMD_SET_DESTINATION   = 0x01;   // 设置飞行目的地
constexpr uint8_t  CMD_SET_ROUTE         = 0x02;   // 设置飞行航线
constexpr uint8_t  CMD_SET_ANGLE         = 0x03;   // 角度控制
constexpr uint8_t  CMD_SET_SPEED         = 0x04;   // 速度控制
constexpr uint8_t  CMD_SET_ALTITUDE      = 0x05;   // 高度控制
constexpr uint8_t  CMD_POWER_SWITCH      = 0x06;   // 载荷电源开关
constexpr uint8_t  CMD_PARACHUTE         = 0x07;   // 开伞控制
constexpr uint8_t  CMD_BUTTON            = 0x09;   // 指令按钮
constexpr uint8_t  CMD_SET_ORIGIN_RETURN = 0x0A;   // 设置原点/返航点
constexpr uint8_t  CMD_SET_GEOFENCE      = 0x0B;   // 设置电子围栏
constexpr uint8_t  CMD_SWITCH_MODE       = 0x0C;   // 切换飞控运行模式
constexpr uint8_t  CMD_GUIDANCE          = 0x0D;   // 末制导指令
constexpr uint8_t  CMD_GUIDANCE_NEW      = 0x0E;   // 末制导指令（新版）
constexpr uint8_t  CMD_GIMBAL_ANG_RATE   = 0x0F;   // 吊舱姿态-角速度模式
constexpr uint8_t  CMD_GIMBAL_ANGLE      = 0x10;   // 吊舱姿态-角度模式
constexpr uint8_t  CMD_TARGET_STATE      = 0x11;   // 识别目标状态

// ---------------------------------------------------------------------------
// 心跳帧固定长度（帧头到帧尾共 89 字节）
// ---------------------------------------------------------------------------
constexpr size_t   HEARTBEAT_FRAME_LEN = 89;

// ---------------------------------------------------------------------------
// 运行模式枚举
// ---------------------------------------------------------------------------
enum class RunMode : uint8_t {
    REAL_FLIGHT  = 0x00,    // 实飞模式
    SIMULATION   = 0x01,    // 仿真模式
};

// ---------------------------------------------------------------------------
// 飞行模式枚举
// ---------------------------------------------------------------------------
enum class FlightMode : uint8_t {
    PRE_TAKEOFF       = 0x00,   // 起飞前
    POST_TAKEOFF      = 0x01,   // 起飞后
    MISSION_READY     = 0x02,   // 任务准备接收
    MISSION_RUNNING   = 0x03,   // 任务执行中
    // 其他值表示故障码
};

// ---------------------------------------------------------------------------
// 飞行状态枚举
// ---------------------------------------------------------------------------
enum class FlightState : uint8_t {
    LEVEL_FLIGHT     = 0x00,   // 横平
    RETURN_HOME      = 0x01,   // 归航
    ORBIT            = 0x02,   // 绕点
    WAYPOINT         = 0x03,   // 指点
    CRUISE           = 0x04,   // 巡航
    TRACKING         = 0x05,   // 跟踪
    LANDING          = 0x06,   // 降落
    TAKEOFF          = 0x07,   // 起飞
    DIVE_PULLUP      = 0x08,   // 俯冲拉起
    EMERGENCY_LAND   = 0x09,   // 迫降
    FAST_DESCENT     = 0x0A,   // 速降
    ORIGIN_LOITER    = 0x0B,   // 原点盘旋
    TAKEOFF_LOCKED   = 0x10,   // 起飞锁定
    TAKEOFF_UNLOCKED = 0x11,   // 起飞解锁
    LAUNCHED         = 0x13,   // 已出筒
    RECOVERY         = 0x20,   // 回收
    LEFT_CIRCLE      = 0x21,   // 左盘
    RIGHT_CIRCLE     = 0x22,   // 右盘
    TOUCHDOWN        = 0x30,   // 着陆
    MISSION_1        = 0xA0,   // 任务1 - 姿态控制模式
    MISSION_2        = 0xA1,   // 任务2 - 单个飞行目的地
    MISSION_3        = 0xA2,   // 任务3 - 按航线飞行
    MISSION_4        = 0xA3,   // 任务4 - 航线结束后绕圈
};

// ---------------------------------------------------------------------------
// 故障信息位定义（U16，每个 bit 代表一类故障）
// ---------------------------------------------------------------------------
struct FaultBits {
    bool link_lost         = false;   // Bit0  - 链路丢失
    bool distance_exceed   = false;   // Bit1  - 距离超限
    bool attitude_exceed   = false;   // Bit2  - 姿态超限
    // Bit3 - 备用
    bool altitude_too_low  = false;   // Bit4  - 高度过低
    bool altitude_too_high = false;   // Bit5  - 高度过高
    bool voltage_too_low   = false;   // Bit6  - 电压过低
    bool mission_fault     = false;   // Bit7  - 任务故障
    bool gps_lost          = false;   // Bit8  - GPS丢失
    bool takeoff_failed    = false;   // Bit9  - 起飞失败
    bool attitude_error    = false;   // Bit10 - 姿态错误
    // Bit11, Bit12 - 备用
    bool return_timeout    = false;   // Bit13 - 归航超时
    bool return_dist_error = false;   // Bit14 - 归航距离异常
    bool parachute_failed  = false;   // Bit15 - 开伞失败

    // 从 uint16 解析故障位
    static FaultBits FromU16(uint16_t val);

    // 转为 uint16
    uint16_t ToU16() const;
};

// ---------------------------------------------------------------------------
// 高度类型枚举
// ---------------------------------------------------------------------------
enum class AltitudeType : uint8_t {
    MSL      = 0x00,   // 海拔高度
    RELATIVE = 0x01,   // 相对高度
};

// ---------------------------------------------------------------------------
// 电源开关命令枚举
// ---------------------------------------------------------------------------
enum class PowerSwitchCmd : uint8_t {
    BATTERY_OFF   = 0x00,   // 关闭电池供电
    BATTERY_ON    = 0x01,   // 开启电池供电
    GIMBAL_OFF    = 0x02,   // 关闭吊舱供电
    GIMBAL_ON     = 0x03,   // 开启吊舱供电
};

// ---------------------------------------------------------------------------
// 开伞类型枚举
// ---------------------------------------------------------------------------
enum class ParachuteType : uint8_t {
    RECOVERY  = 0x00,   // 回收开伞
    IMMEDIATE = 0x01,   // 立即开伞
};

// ---------------------------------------------------------------------------
// 坐标点类型枚举
// ---------------------------------------------------------------------------
enum class CoordPointType : uint8_t {
    ORIGIN      = 0x00,   // 原点
    RETURN_HOME = 0x01,   // 返航点
};

// ===========================================================================
// 协议数据结构定义
// ===========================================================================

// ---- 飞控心跳状态（飞控→域控，帧类型 0x01）----
struct HeartbeatData {
    uint8_t  cnt              = 0;        // 帧计数
    uint8_t  aircraft_id      = 0;        // 飞机ID
    uint8_t  run_mode         = 0;        // 运行模式
    uint8_t  satellite_count  = 0;        // 定位星数
    int32_t  longitude        = 0;        // 经度 (×1e-7)
    int32_t  latitude         = 0;        // 纬度 (×1e-7)
    uint16_t altitude         = 0;        // 海拔高度 (×0.1m)
    uint16_t relative_alt     = 0;        // 相对高度 (×0.1m)
    uint16_t airspeed         = 0;        // 空速 (×1e-2 m/s)
    uint16_t groundspeed      = 0;        // 地速 (×1e-2 m/s)
    int16_t  velocity_x       = 0;        // 飞行速度x (×1e-2 m/s)
    int16_t  velocity_y       = 0;        // 飞行速度y (×1e-2 m/s)
    int16_t  velocity_z       = 0;        // 飞行速度z (×1e-2 m/s)
    int16_t  accel_x          = 0;        // 加速度x (×1e-2 m/s²)
    int16_t  accel_y          = 0;        // 加速度y (×1e-2 m/s²)
    int16_t  accel_z          = 0;        // 加速度z (×1e-2 m/s²)
    int16_t  roll             = 0;        // 横滚角 (×1e-2 度)
    int16_t  pitch            = 0;        // 俯仰角 (×1e-2 度)
    uint16_t yaw              = 0;        // 偏航角 (×1e-2 度)
    uint8_t  flight_mode      = 0;        // 飞行模式
    uint16_t current_waypoint = 0;        // 当前航迹点
    uint16_t battery_voltage  = 0;        // 电池电压 (×1e-2 V)
    uint8_t  battery_percent  = 0;        // 电池电量 (百分比)
    uint16_t battery_current  = 0;        // 电池电流 (×1e-1 A)
    uint16_t atm_pressure     = 0;        // 大气压强 (×1e-2 kPa)
    uint64_t utc_timestamp    = 0;        // UTC时间戳
    uint16_t fault_info       = 0;        // 故障信息位
    uint16_t rotation_speed   = 0;        // 转速 (角度/s)
    uint16_t throttle         = 0;        // 油门量
    uint16_t target_altitude  = 0;        // 目标高度 (×0.1m)
    uint16_t target_speed     = 0;        // 目标速度 (×1e-2 m/s)
    uint16_t origin_distance  = 0;        // 原点距离 (m)
    uint16_t origin_heading   = 0;        // 原点航向 (×1e-2 度)
    uint16_t target_distance  = 0;        // 目标距离 (m)
    uint8_t  flight_state     = 0;        // 飞行状态
    uint8_t  altitude_state   = 0;        // 高度状态
    uint8_t  state_switch_src = 0;        // 状态切换源
    uint16_t flight_time      = 0;        // 航时 (秒)
    uint16_t flight_range     = 0;        // 航程 (米)
    uint32_t reserved         = 0;        // 备用4字节
};

// ---- 飞控控制云台转动指令（飞控→域控，帧类型 0x30）----
struct GimbalControlData {
    uint8_t  cnt             = 0;     // 帧计数
    uint8_t  enable          = 0;     // 云台控制使能 (0=不使能, 1=使能)
    int16_t  pitch_angle     = 0;     // 云台俯仰角度 (×1e-2 度)
    int16_t  yaw_angle       = 0;     // 云台偏航角度 (×1e-2 度)
};

// ---- 指令回复（飞控→域控，帧类型 0x20）----
struct CommandReplyData {
    uint8_t  cnt             = 0;     // 帧计数
    uint8_t  replied_cmd     = 0;     // 回复的指令类型
    uint8_t  result          = 0;     // 结果 (0x00=成功, 0x01=失败)
};

// ---- 设置飞行目的地（域控→飞控，帧类型 0x02）----
struct SetDestinationData {
    uint8_t  cmd_type  = CMD_SET_DESTINATION;  // 指令类型 0x01
    int32_t  longitude = 0;     // 经度 (×1e-7)
    int32_t  latitude  = 0;     // 纬度 (×1e-7)
    uint16_t altitude  = 0;     // 高度 (×0.1m)
};

// ---- 航线点 ----
struct RoutePoint {
    int32_t  longitude = 0;     // 经度 (×1e-7)
    int32_t  latitude  = 0;     // 纬度 (×1e-7)
    uint16_t altitude  = 0;     // 高度 (×0.1m)
    uint16_t index     = 0;     // 航线点序号
    uint16_t speed     = 0;     // 飞行速度 (×1e-2 m/s)
};

// ---- 设置飞行航线（域控→飞控，帧类型 0x10, 指令 0x02）----
struct SetRouteData {
    uint8_t cmd_type = CMD_SET_ROUTE;      // 指令类型 0x02
    std::vector<RoutePoint> points;        // 航线点列表 (1~50个)
};

// ---- 设置角度（域控→飞控，帧类型 0x10, 指令 0x03）----
struct SetAngleData {
    uint8_t  cmd_type = CMD_SET_ANGLE;
    int16_t  pitch    = 0;      // 俯仰角 (×1e-2 度)
    uint16_t yaw      = 0;      // 偏航角 (×1e-2 度)
};

// ---- 设置飞行速度（域控→飞控，帧类型 0x10, 指令 0x04）----
struct SetSpeedData {
    uint8_t  cmd_type = CMD_SET_SPEED;
    uint16_t speed    = 0;      // 飞行速度 (×1e-2 m/s)
};

// ---- 设置飞行高度（域控→飞控，帧类型 0x10, 指令 0x05）----
struct SetAltitudeData {
    uint8_t  cmd_type     = CMD_SET_ALTITUDE;
    uint8_t  altitude_type = 0;   // 高度类型 (0=海拔, 1=相对)
    uint16_t altitude      = 0;   // 高度 (×0.1m)
};

// ---- 电源开关（域控→飞控，帧类型 0x10, 指令 0x06）----
struct PowerSwitchData {
    uint8_t cmd_type = CMD_POWER_SWITCH;
    uint8_t command  = 0;       // 开关命令
};

// ---- 开伞控制（域控→飞控，帧类型 0x10, 指令 0x07）----
struct ParachuteData {
    uint8_t cmd_type       = CMD_PARACHUTE;
    uint8_t parachute_type = 0;   // 开伞类型
};

// ---- 指令按钮（域控→飞控，帧类型 0x10, 指令 0x09）----
struct ButtonCommandData {
    uint8_t cmd_type = CMD_BUTTON;
    uint8_t button   = 0;       // 指令按钮
};

// ---- 设置原点/返航点（域控→飞控，帧类型 0x10, 指令 0x0A）----
struct SetOriginReturnData {
    uint8_t  cmd_type   = CMD_SET_ORIGIN_RETURN;
    uint8_t  point_type = 0;      // 坐标点类型 (0=原点, 1=返航点)
    int32_t  longitude  = 0;      // 经度 (×1e-7)
    int32_t  latitude   = 0;      // 纬度 (×1e-7)
    uint16_t altitude   = 0;      // 高度 (×0.1m)
};

// ---- 电子围栏点 ----
struct GeofencePoint {
    int32_t  longitude = 0;     // 经度 (×1e-7)
    int32_t  latitude  = 0;     // 纬度 (×1e-7)
    uint16_t altitude  = 0;     // 高度 (×0.1m)
    uint8_t  index     = 0;     // 点序号
};

// ---- 设置电子围栏（域控→飞控，帧类型 0x10, 指令 0x0B）----
struct SetGeofenceData {
    uint8_t cmd_type = CMD_SET_GEOFENCE;
    std::vector<GeofencePoint> points;   // 围栏点列表 (3~50个)
};

// ---- 切换运行模式（域控→飞控，帧类型 0x10, 指令 0x0C）----
struct SwitchModeData {
    uint8_t cmd_type = CMD_SWITCH_MODE;
    uint8_t mode     = 0;       // 0=实飞, 1=仿真
};

// ---- 末制导指令（域控→飞控，帧类型 0x10, 指令 0x0D）----
struct GuidanceData {
    uint8_t  cmd_type       = CMD_GUIDANCE;
    uint8_t  mode_switch    = 0;      // 0=使用id打击, 1=使用经纬度打击
    uint32_t frame_id       = 0;      // 目标帧id
    uint32_t track_id       = 0;      // 目标跟踪id
    int32_t  target_lon     = 0;      // 目标经度 (×1e-7)
    int32_t  target_lat     = 0;      // 目标纬度 (×1e-7)
    uint16_t target_alt     = 0;      // 目标海拔 (×0.1m)
};

// ---- 末制导指令-新版（域控→飞控，帧类型 0x10, 指令 0x0E）----
struct GuidanceNewData {
    uint8_t  cmd_type             = CMD_GUIDANCE_NEW;
    int16_t  pitch_los_rate       = 0;    // 俯仰视线角速率 (×1e-2)
    int16_t  yaw_los_rate         = 0;    // 偏航视线角速率 (×1e-2)
    int16_t  pitch_los_angle      = 0;    // 俯仰视线角度 (×1e-2)
    int16_t  yaw_los_angle        = 0;    // 偏航视线角度 (×1e-2)
    int16_t  pitch_frame_angle    = 0;    // 俯仰框架角度 (×1e-2)
    int16_t  yaw_frame_angle      = 0;    // 偏航框架角度 (×1e-2)
    int16_t  gimbal_pitch_angle   = 0;    // 吊舱俯仰角度 (×1e-2)
    int16_t  gimbal_yaw_angle     = 0;    // 吊舱偏航角度 (×1e-2)
    int16_t  acc_x                = 0;    // AccX (×1e-2)
    int16_t  acc_y                = 0;    // AccY (×1e-2)
    int16_t  acc_z                = 0;    // AccZ (×1e-2)
    int16_t  gyro_x               = 0;    // GyroX (×1e-2)
    int16_t  gyro_y               = 0;    // GyroY (×1e-2)
    int16_t  gyro_z               = 0;    // GyroZ (×1e-2)
    uint16_t target_id            = 0;    // 目标ID
    uint16_t target_type          = 0;    // 目标类型
    int32_t  target_lon           = 0;    // 目标经度 (×1e-7)
    int32_t  target_lat           = 0;    // 目标纬度 (×1e-7)
    int32_t  target_alt           = 0;    // 目标高度 (×0.1)
    int32_t  laser_range          = 0;    // 激光测距 (×0.1)
    uint8_t  status               = 0;    // 0=搜寻, 1=锁定
};

// ---- 吊舱姿态-角速度模式（域控→飞控，帧类型 0x10, 指令 0x0F）----
struct GimbalAngRateData {
    uint8_t cmd_type           = CMD_GIMBAL_ANG_RATE;
    int32_t pitch_los_rate     = 0;    // 俯仰视线角速率 (×1e-3)
    int32_t yaw_los_rate       = 0;    // 偏航视线角速率 (×1e-3)
    int32_t target_lon         = 0;    // 目标经度 (×1e-7)
    int32_t target_lat         = 0;    // 目标纬度 (×1e-7)
    int32_t target_alt         = 0;    // 目标高度 (×1e-3)
    int32_t laser_range        = 0;    // 激光测距 (×1e-3)
    uint8_t status             = 0;    // 0=搜寻, 1=锁定
};

// ---- 吊舱姿态-角度模式（域控→飞控，帧类型 0x10, 指令 0x10）----
struct GimbalAngleData {
    uint8_t cmd_type           = CMD_GIMBAL_ANGLE;
    int32_t pitch_frame_angle  = 0;    // 俯仰框架角 (×1e-3)
    int32_t yaw_frame_angle    = 0;    // 偏航框架角 (×1e-3)
    int32_t pitch_ang_rate     = 0;    // 俯仰角速度 (×1e-3)
    int32_t yaw_ang_rate       = 0;    // 偏航角速度 (×1e-3)
};

// ---- 识别目标状态（域控→飞控，帧类型 0x10, 指令 0x11）----
struct TargetStateData {
    uint8_t cmd_type                = CMD_TARGET_STATE;
    int32_t pitch_axis_angle        = 0;    // 光轴和目标视线俯仰角度 (×1e-3)
    int32_t yaw_axis_angle          = 0;    // 光轴和目标视线偏航角度 (×1e-3)
    uint8_t target_vertical_ratio   = 0;    // 目标竖向在图像中占比 (1~100)
    uint8_t target_horizontal_ratio = 0;    // 目标横向在图像中占比 (1~100)
    uint8_t status                  = 0;    // 0=搜寻, 1=锁定
};

// ---------------------------------------------------------------------------
// 通用解析后帧结构（帧层输出）
// ---------------------------------------------------------------------------
struct ParsedFrame {
    uint8_t              cnt        = 0;      // 帧计数
    uint8_t              frame_type = 0;      // 帧类型
    std::vector<uint8_t> payload;             // 载荷（帧头+CNT+帧类型 之后，校验和之前的部分）
    uint8_t              checksum   = 0;      // 收到的校验和
    bool                 valid      = false;  // 校验是否通过
};

} // namespace fly_control
