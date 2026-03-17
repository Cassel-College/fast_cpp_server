#include "MyFlyControl.h"
#include "MyLog.h"

#include <chrono>

// =============================================================================
// MyFlyControl 业务层实现
// =============================================================================

namespace fly_control {

MyFlyControl::MyFlyControl() = default;

MyFlyControl::~MyFlyControl() {
    Stop();
}

// =============================================================================
// 初始化与生命周期
// =============================================================================

bool MyFlyControl::Init(const nlohmann::json& cfg, std::string* err) {
    // 通过 MySerial 初始化串口
    if (!serial_.Init(cfg, err)) {
        MYLOG_ERROR("飞控模块串口初始化失败: {}",
                    (err != nullptr && !err->empty()) ? *err : std::string("unknown error"));
        return false;
    }
    MYLOG_INFO("飞控模块串口初始化成功");
    return true;
}

bool MyFlyControl::Start(std::string* err) {
    if (running_.load()) {
        if (err) *err = "飞控模块已在运行";
        return false;
    }

    // 确保串口已打开
    if (!serial_.IsOpen()) {
        if (!serial_.Open(err)) {
            MYLOG_ERROR("飞控模块串口打开失败: {}",
                        (err != nullptr && !err->empty()) ? *err : std::string("unknown error"));
            return false;
        }
    } else {
        MYLOG_INFO("飞控模块串口已打开");
    }

    // 重置帧解析器
    parser_.Reset();

    // 启动接收线程
    running_.store(true);
    recv_thread_ = std::make_unique<std::thread>(&MyFlyControl::ReceiveLoop, this);

    MYLOG_INFO("飞控模块已启动");
    return true;
}

void MyFlyControl::Stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);

    // 等待接收线程退出
    if (recv_thread_ && recv_thread_->joinable()) {
        recv_thread_->join();
    }
    recv_thread_.reset();

    serial_.Close();
    MYLOG_INFO("飞控模块已停止");
}

bool MyFlyControl::IsRunning() const {
    return running_.load();
}

// =============================================================================
// 回调注册
// =============================================================================

void MyFlyControl::SetOnHeartbeat(HeartbeatCallback cb) {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    on_heartbeat_ = std::move(cb);
}

void MyFlyControl::SetOnCommandReply(CommandReplyCallback cb) {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    on_reply_ = std::move(cb);
}

void MyFlyControl::SetOnGimbalControl(GimbalControlCallback cb) {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    on_gimbal_ = std::move(cb);
}

// =============================================================================
// 状态查询
// =============================================================================

HeartbeatData MyFlyControl::GetLatestHeartbeat() const {
    std::lock_guard<std::mutex> lock(hb_mutex_);
    return latest_hb_;
}

nlohmann::json MyFlyControl::GetLatestHeartbeatJson() const {
    std::lock_guard<std::mutex> lock(hb_mutex_);
    return HeartbeatToJson(latest_hb_);
}

FaultBits MyFlyControl::GetFaultBits() const {
    std::lock_guard<std::mutex> lock(hb_mutex_);
    return FaultBits::FromU16(latest_hb_.fault_info);
}

bool MyFlyControl::HasHeartbeat() const {
    std::lock_guard<std::mutex> lock(hb_mutex_);
    return has_hb_;
}

// =============================================================================
// 发送控制指令
// =============================================================================

bool MyFlyControl::SendSetDestination(int32_t lon, int32_t lat, uint16_t alt,
                                       std::string* err) {
    SetDestinationData data;
    data.longitude = lon;
    data.latitude  = lat;
    data.altitude  = alt;
    auto frame = EncodeSetDestination(NextCnt(), data);
    MYLOG_INFO("发送指令: 设置飞行目的地");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSetRoute(const std::vector<RoutePoint>& points,
                                 std::string* err) {
    if (points.empty() || points.size() > 50) {
        if (err) *err = "航线点数量必须在 1~50 之间";
        return false;
    }
    SetRouteData data;
    data.points = points;
    auto frame = EncodeSetRoute(NextCnt(), data);
    MYLOG_INFO("发送指令: 设置飞行航线, 航线点数={}", points.size());
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSetAngle(int16_t pitch, uint16_t yaw, std::string* err) {
    SetAngleData data;
    data.pitch = pitch;
    data.yaw   = yaw;
    auto frame = EncodeSetAngle(NextCnt(), data);
    MYLOG_INFO("发送指令: 角度控制");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSetSpeed(uint16_t speed, std::string* err) {
    SetSpeedData data;
    data.speed = speed;
    auto frame = EncodeSetSpeed(NextCnt(), data);
    MYLOG_INFO("发送指令: 速度控制, speed={}", speed);
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSetAltitude(uint8_t alt_type, uint16_t altitude,
                                    std::string* err) {
    SetAltitudeData data;
    data.altitude_type = alt_type;
    data.altitude      = altitude;
    auto frame = EncodeSetAltitude(NextCnt(), data);
    MYLOG_INFO("发送指令: 高度控制");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendPowerSwitch(uint8_t command, std::string* err) {
    PowerSwitchData data;
    data.command = command;
    auto frame = EncodePowerSwitch(NextCnt(), data);
    MYLOG_INFO("发送指令: 电源开关, cmd=0x{:02X}", command);
    return SendRawData(frame, err);
}

bool MyFlyControl::SendParachute(uint8_t parachute_type, std::string* err) {
    ParachuteData data;
    data.parachute_type = parachute_type;
    auto frame = EncodeParachute(NextCnt(), data);
    MYLOG_INFO("发送指令: 开伞控制, type=0x{:02X}", parachute_type);
    return SendRawData(frame, err);
}

bool MyFlyControl::SendButtonCommand(uint8_t button, std::string* err) {
    ButtonCommandData data;
    data.button = button;
    auto frame = EncodeButtonCommand(NextCnt(), data);
    MYLOG_INFO("发送指令: 指令按钮=0x{:02X}", button);
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSetOriginReturn(uint8_t point_type, int32_t lon,
                                        int32_t lat, uint16_t alt,
                                        std::string* err) {
    SetOriginReturnData data;
    data.point_type = point_type;
    data.longitude  = lon;
    data.latitude   = lat;
    data.altitude   = alt;
    auto frame = EncodeSetOriginReturn(NextCnt(), data);
    MYLOG_INFO("发送指令: 设置{}", point_type == 0 ? "原点" : "返航点");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSetGeofence(const std::vector<GeofencePoint>& points,
                                    std::string* err) {
    if (points.size() < 3 || points.size() > 50) {
        if (err) *err = "电子围栏点数量必须在 3~50 之间";
        return false;
    }
    SetGeofenceData data;
    data.points = points;
    auto frame = EncodeSetGeofence(NextCnt(), data);
    MYLOG_INFO("发送指令: 设置电子围栏, 点数={}", points.size());
    return SendRawData(frame, err);
}

bool MyFlyControl::SendSwitchMode(uint8_t mode, std::string* err) {
    SwitchModeData data;
    data.mode = mode;
    auto frame = EncodeSwitchMode(NextCnt(), data);
    MYLOG_INFO("发送指令: 切换运行模式=0x{:02X}", mode);
    return SendRawData(frame, err);
}

bool MyFlyControl::SendGuidance(const GuidanceData& data, std::string* err) {
    auto frame = EncodeGuidance(NextCnt(), data);
    MYLOG_INFO("发送指令: 末制导指令");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendGuidanceNew(const GuidanceNewData& data, std::string* err) {
    auto frame = EncodeGuidanceNew(NextCnt(), data);
    MYLOG_INFO("发送指令: 末制导指令(新版)");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendGimbalAngRate(const GimbalAngRateData& data, std::string* err) {
    auto frame = EncodeGimbalAngRate(NextCnt(), data);
    MYLOG_INFO("发送指令: 吊舱姿态-角速度模式");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendGimbalAngle(const GimbalAngleData& data, std::string* err) {
    auto frame = EncodeGimbalAngle(NextCnt(), data);
    MYLOG_INFO("发送指令: 吊舱姿态-角度模式");
    return SendRawData(frame, err);
}

bool MyFlyControl::SendTargetState(const TargetStateData& data, std::string* err) {
    auto frame = EncodeTargetState(NextCnt(), data);
    MYLOG_INFO("发送指令: 识别目标状态");
    return SendRawData(frame, err);
}

// =============================================================================
// 内部方法
// =============================================================================

void MyFlyControl::ReceiveLoop() {
    MYLOG_INFO("飞控接收线程启动");
    constexpr size_t READ_BUF_SIZE = 256;

    while (running_.load()) {
        // 从串口读取数据
        std::string err;
        auto bytes = serial_.ReadBytes(READ_BUF_SIZE, &err);

        if (bytes.empty()) {
            // 无数据可读，短暂休眠避免 CPU 空转
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // 喂入帧解析器
        parser_.FeedData(bytes);

        // 尝试取出所有可用帧
        ParsedFrame frame;
        while (parser_.PopFrame(frame)) {
            if (!frame.valid) {
                MYLOG_WARN("收到校验和不通过的帧, 帧类型=0x{:02X}, 丢弃",
                           frame.frame_type);
                continue;
            }
            HandleFrame(frame);
        }
    }

    MYLOG_INFO("飞控接收线程退出");
}

void MyFlyControl::HandleFrame(const ParsedFrame& frame) {
    switch (frame.frame_type) {
        case FRAME_TYPE_HEARTBEAT: {
            HeartbeatData hb;
            hb.cnt = frame.cnt;
            if (DecodeHeartbeat(frame.payload, hb)) {
                // 更新最新心跳数据
                {
                    std::lock_guard<std::mutex> lock(hb_mutex_);
                    latest_hb_ = hb;
                    has_hb_ = true;
                }
                // 触发回调
                std::lock_guard<std::mutex> lock(cb_mutex_);
                if (on_heartbeat_) {
                    on_heartbeat_(hb);
                }
            } else {
                MYLOG_WARN("心跳帧解码失败");
            }
            break;
        }

        case FRAME_TYPE_REPLY: {
            CommandReplyData reply;
            reply.cnt = frame.cnt;
            if (DecodeCommandReply(frame.payload, reply)) {
                MYLOG_INFO("收到指令回复: {}={}", GetCommandName(reply.replied_cmd),
                           reply.result == 0 ? "成功" : "失败");
                std::lock_guard<std::mutex> lock(cb_mutex_);
                if (on_reply_) {
                    on_reply_(reply);
                }
            } else {
                MYLOG_WARN("指令回复帧解码失败");
            }
            break;
        }

        case FRAME_TYPE_GIMBAL_CONTROL: {
            GimbalControlData gimbal;
            gimbal.cnt = frame.cnt;
            if (DecodeGimbalControl(frame.payload, gimbal)) {
                std::lock_guard<std::mutex> lock(cb_mutex_);
                if (on_gimbal_) {
                    on_gimbal_(gimbal);
                }
            } else {
                MYLOG_WARN("云台控制帧解码失败");
            }
            break;
        }

        default:
            MYLOG_WARN("收到未知帧类型: 0x{:02X}", frame.frame_type);
            break;
    }
}

bool MyFlyControl::SendRawData(const std::vector<uint8_t>& data, std::string* err) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    size_t written = serial_.Write(data, err);
    return written == data.size();
}

uint8_t MyFlyControl::NextCnt() {
    return cnt_.fetch_add(1);
}

nlohmann::json MyFlyControl::HeartbeatToJson(const HeartbeatData& hb) {
    nlohmann::json j;
    j["cnt"]              = hb.cnt;
    j["aircraft_id"]      = hb.aircraft_id;
    j["run_mode"]         = hb.run_mode;
    j["satellite_count"]  = hb.satellite_count;
    j["longitude"]        = hb.longitude * 1e-7;
    j["latitude"]         = hb.latitude * 1e-7;
    j["altitude"]         = hb.altitude * 0.1;
    j["relative_alt"]     = hb.relative_alt * 0.1;
    j["airspeed"]         = hb.airspeed * 0.01;
    j["groundspeed"]      = hb.groundspeed * 0.01;
    j["velocity_x"]       = hb.velocity_x * 0.01;
    j["velocity_y"]       = hb.velocity_y * 0.01;
    j["velocity_z"]       = hb.velocity_z * 0.01;
    j["accel_x"]          = hb.accel_x * 0.01;
    j["accel_y"]          = hb.accel_y * 0.01;
    j["accel_z"]          = hb.accel_z * 0.01;
    j["roll"]             = hb.roll * 0.01;
    j["pitch"]            = hb.pitch * 0.01;
    j["yaw"]              = hb.yaw * 0.01;
    j["flight_mode"]      = hb.flight_mode;
    j["current_waypoint"] = hb.current_waypoint;
    j["battery_voltage"]  = hb.battery_voltage * 0.01;
    j["battery_percent"]  = hb.battery_percent;
    j["battery_current"]  = hb.battery_current * 0.1;
    j["atm_pressure"]     = hb.atm_pressure * 0.01;
    j["utc_timestamp"]    = hb.utc_timestamp;
    j["fault_info"]       = hb.fault_info;
    j["rotation_speed"]   = hb.rotation_speed;
    j["throttle"]         = hb.throttle;
    j["target_altitude"]  = hb.target_altitude * 0.1;
    j["target_speed"]     = hb.target_speed * 0.01;
    j["origin_distance"]  = hb.origin_distance;
    j["origin_heading"]   = hb.origin_heading * 0.01;
    j["target_distance"]  = hb.target_distance;
    j["flight_state"]     = hb.flight_state;
    j["altitude_state"]   = hb.altitude_state;
    j["state_switch_src"] = hb.state_switch_src;
    j["flight_time"]      = hb.flight_time;
    j["flight_range"]     = hb.flight_range;
    return j;
}

} // namespace fly_control
