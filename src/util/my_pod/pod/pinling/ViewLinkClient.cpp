#include "ViewLinkClient.h"

#include <cstring>
#include <stdexcept>
#include <sys/stat.h>

#include "ViewLink.h"

namespace viewlink {

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

ViewLinkClient::ViewLinkClient() = default;

ViewLinkClient::~ViewLinkClient()
{
    Shutdown();
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

void ViewLinkClient::Initialize()
{
    if (initialized_) return;

    int ret = VLK_Init();
    if (ret != VLK_ERROR_NO_ERROR) {
        throw std::runtime_error("VLK_Init failed, error: " + std::to_string(ret));
    }

    VLK_RegisterDevStatusCB(OnDeviceStatus, this);

    initialized_ = true;
}

void ViewLinkClient::Shutdown()
{
    if (!initialized_) return;

    Disconnect();
    VLK_UnInit();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        telemetry_   = TelemetrySnapshot();
        model_info_  = DeviceModelInfo();
        config_info_ = DeviceConfigInfo();
    }

    connected_.store(false);
    tcp_connected_.store(false);
    serial_connected_.store(false);
    initialized_ = false;
}

// ---------------------------------------------------------------------------
// 连接管理
// ---------------------------------------------------------------------------

void ViewLinkClient::Connect(const ConnectionConfig& config)
{
    VLK_CONN_PARAM param;
    std::memset(&param, 0, sizeof(param));

    if (config.type == ConnectionType::Tcp) {
        param.emType = VLK_CONN_TYPE_TCP;
        std::strncpy(param.ConnParam.IPAddr.szIPV4,
                     config.ip.c_str(),
                     sizeof(param.ConnParam.IPAddr.szIPV4) - 1);
        param.ConnParam.IPAddr.iPort = config.port;
    } else {
        param.emType = VLK_CONN_TYPE_SERIAL_PORT;
        std::strncpy(param.ConnParam.SerialPort.szSerialPortName,
                     config.serial_port.c_str(),
                     sizeof(param.ConnParam.SerialPort.szSerialPortName) - 1);
        param.ConnParam.SerialPort.iBaudRate = config.baud_rate;
    }

    int ret = VLK_Connect(&param, OnConnectionStatus, this);
    if (ret != VLK_ERROR_NO_ERROR) {
        throw std::runtime_error("VLK_Connect failed, error: " + std::to_string(ret));
    }
}

void ViewLinkClient::Disconnect()
{
    VLK_Disconnect();
    connected_.store(false);
    tcp_connected_.store(false);
    serial_connected_.store(false);
}

bool ViewLinkClient::WaitForConnection(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return connection_cv_.wait_for(lock, timeout, [this]{ return connected_.load(); });
}

bool ViewLinkClient::WaitForTelemetry(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return telemetry_cv_.wait_for(lock, timeout, [this]{ return telemetry_.valid; });
}

// ---------------------------------------------------------------------------
// 连接状态查询
// ---------------------------------------------------------------------------

bool ViewLinkClient::IsConnected()       const { return connected_.load(); }
bool ViewLinkClient::IsTcpConnected()    const { return tcp_connected_.load(); }
bool ViewLinkClient::IsSerialConnected() const { return serial_connected_.load(); }

std::string ViewLinkClient::GetConnectionTypeName() const
{
    if (tcp_connected_.load())    return "TCP";
    if (serial_connected_.load()) return "Serial";
    return "None";
}

// ---------------------------------------------------------------------------
// 吊舱姿态 / 遥测数据读取 (线程安全)
// ---------------------------------------------------------------------------

Attitude ViewLinkClient::GetAttitude() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return telemetry_.attitude;
}

TelemetrySnapshot ViewLinkClient::GetTelemetry() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return telemetry_;
}

DeviceModelInfo ViewLinkClient::GetModelInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return model_info_;
}

DeviceConfigInfo ViewLinkClient::GetConfigInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_info_;
}

bool ViewLinkClient::HasTelemetry() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return telemetry_.valid;
}

bool ViewLinkClient::HasModelInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return model_info_.valid;
}

bool ViewLinkClient::HasConfigInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_info_.valid;
}

void ViewLinkClient::SetTelemetryCallback(TelemetryCallback cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    telemetry_cb_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// 云台运动控制
// ---------------------------------------------------------------------------

void ViewLinkClient::Move(short horizontal_speed, short vertical_speed)
{
    VLK_Move(horizontal_speed, vertical_speed);
}

void ViewLinkClient::StopMove() { VLK_Stop(); }
void ViewLinkClient::Home()     { VLK_Home(); }

void ViewLinkClient::TurnTo(double yaw, double pitch)
{
    VLK_TurnTo(yaw, pitch);
}


// ---------------------------------------------------------------------------
// 变焦控制
// ---------------------------------------------------------------------------

void ViewLinkClient::ZoomIn(short speed)           { VLK_ZoomIn(speed); }
void ViewLinkClient::ZoomOut(short speed)           { VLK_ZoomOut(speed); }
void ViewLinkClient::StopZoom()                     { VLK_StopZoom(); }
void ViewLinkClient::ZoomTo(float magnification)    { VLK_ZoomTo(magnification); }

// ---------------------------------------------------------------------------
// 对焦控制
// ---------------------------------------------------------------------------

void ViewLinkClient::FocusIn(short speed)           { VLK_FocusIn(speed); }
void ViewLinkClient::FocusOut(short speed)           { VLK_FocusOut(speed); }
void ViewLinkClient::StopFocus()                     { VLK_StopFocus(); }
void ViewLinkClient::SetFocusMode(int mode)          { VLK_SetFocusMode(static_cast<VLK_FOCUS_MODE>(mode)); }
int  ViewLinkClient::GetFocusMode() const            { return VLK_GetFocusMode(); }

// ---------------------------------------------------------------------------
// 跟踪控制
// ---------------------------------------------------------------------------

void ViewLinkClient::StartTracking(int x, int y, int video_width, int video_height)
{
    VLK_TRACK_MODE_PARAM param;
    std::memset(&param, 0, sizeof(param));
    param.emTrackSensor   = VLK_SENSOR_VISIBLE1;
    param.emTrackTempSize = VLK_TRACK_TEMPLATE_SIZE_AUTO;
    VLK_TrackTargetPositionEx(&param, x, y, video_width, video_height);
}

void ViewLinkClient::StopTracking()   { VLK_DisableTrackMode(); }
bool ViewLinkClient::IsTracking() const { return VLK_IsTracking() != 0; }

// ---------------------------------------------------------------------------
// 电机 / 跟随
// ---------------------------------------------------------------------------

void ViewLinkClient::SwitchMotor(bool on)   { VLK_SwitchMotor(on ? 1 : 0); }
bool ViewLinkClient::IsMotorOn() const      { return VLK_IsMotorOn() != 0; }
void ViewLinkClient::SetFollowMode(bool en) { VLK_EnableFollowMode(en ? 1 : 0); }
bool ViewLinkClient::IsFollowMode() const   { return VLK_IsFollowMode() != 0; }

// ---------------------------------------------------------------------------
// 录像 / 拍照
// ---------------------------------------------------------------------------

void ViewLinkClient::Photograph()          { VLK_Photograph(); }
void ViewLinkClient::SwitchRecord(bool on) { VLK_SwitchRecord(on ? 1 : 0); }
bool ViewLinkClient::IsRecording() const   { return VLK_IsRecording() != 0; }

// ---------------------------------------------------------------------------
// 激光
// ---------------------------------------------------------------------------

void ViewLinkClient::SwitchLaser(bool on)  { VLK_SwitchLaser(on ? 1 : 0); }

// ---------------------------------------------------------------------------
// 其它
// ---------------------------------------------------------------------------

std::string ViewLinkClient::GetSdkVersion() const
{
    return GetSDKVersion();
}

// ---------------------------------------------------------------------------
// SDK 回调 (static)
// ---------------------------------------------------------------------------

int ViewLinkClient::OnConnectionStatus(int status, const char* /*msg*/, int /*len*/, void* user)
{
    auto* self = static_cast<ViewLinkClient*>(user);

    switch (status) {
    case VLK_CONN_STATUS_TCP_CONNECTED:
        self->tcp_connected_.store(true);
        self->connected_.store(true);
        break;
    case VLK_CONN_STATUS_TCP_DISCONNECTED:
        self->tcp_connected_.store(false);
        self->connected_.store(self->serial_connected_.load());
        break;
    case VLK_CONN_STATUS_SERIAL_PORT_CONNECTED:
        self->serial_connected_.store(true);
        self->connected_.store(true);
        break;
    case VLK_CONN_STATUS_SERIAL_PORT_DISCONNECTED:
        self->serial_connected_.store(false);
        self->connected_.store(self->tcp_connected_.load());
        break;
    default:
        self->connected_.store(false);
        self->tcp_connected_.store(false);
        self->serial_connected_.store(false);
        break;
    }

    self->connection_cv_.notify_all();
    return 0;
}

int ViewLinkClient::OnDeviceStatus(int type, const char* buf, int /*len*/, void* user)
{
    auto* self = static_cast<ViewLinkClient*>(user);
    if (!buf) return 0;

    // 直接在回调中处理 (和原始 demo 一致, 赋值操作在 mutex 下足够快)
    if (type == VLK_DEV_STATUS_TYPE_MODEL) {
        const VLK_DEV_MODEL* m = reinterpret_cast<const VLK_DEV_MODEL*>(buf);
        std::lock_guard<std::mutex> lock(self->mutex_);
        self->model_info_.model_code = static_cast<int>(m->cModelCode);
        self->model_info_.model_name = std::string(m->szModelName,
            strnlen(m->szModelName, sizeof(m->szModelName)));
        self->model_info_.valid = true;
    }
    else if (type == VLK_DEV_STATUS_TYPE_CONFIG) {
        const VLK_DEV_CONFIG* c = reinterpret_cast<const VLK_DEV_CONFIG*>(buf);
        std::lock_guard<std::mutex> lock(self->mutex_);
        self->config_info_.version_no = std::string(c->cVersionNO,
            strnlen(c->cVersionNO, sizeof(c->cVersionNO)));
        self->config_info_.device_id = std::string(c->cDeviceID,
            strnlen(c->cDeviceID, sizeof(c->cDeviceID)));
        self->config_info_.serial_no = std::string(c->cSerialNO,
            strnlen(c->cSerialNO, sizeof(c->cSerialNO)));
        self->config_info_.valid = true;
    }
    else if (type == VLK_DEV_STATUS_TYPE_TELEMETRY) {
        const VLK_DEV_TELEMETRY* t = reinterpret_cast<const VLK_DEV_TELEMETRY*>(buf);
        TelemetrySnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(self->mutex_);
            self->telemetry_.attitude.yaw   = t->dYaw;
            self->telemetry_.attitude.pitch = t->dPitch;
            self->telemetry_.attitude.roll  = t->dRoll;
            self->telemetry_.attitude.valid = true;

            self->telemetry_.sensor_type        = static_cast<int>(t->emSensorType);
            self->telemetry_.tracker_status     = static_cast<int>(t->emTrackerStatus);
            self->telemetry_.target_lat         = t->dTargetLat;
            self->telemetry_.target_lng         = t->dTargetLng;
            self->telemetry_.target_alt         = t->dTargetAlt;
            self->telemetry_.drone_lat          = t->dDroneLat;
            self->telemetry_.drone_lng          = t->dDroneLng;
            self->telemetry_.drone_alt          = t->dDroneAlt;
            self->telemetry_.horizontal_fov     = t->dFov;
            self->telemetry_.vertical_fov       = t->dVertical;
            self->telemetry_.eo_digital_zoom    = t->iEODigitalZoom;
            self->telemetry_.ir_digital_zoom    = t->iIRDigitalZoom;
            self->telemetry_.zoom_magnification = t->dZoomMagTimes;
            self->telemetry_.laser_distance     = t->sLaserDistance;
            self->telemetry_.ir_color           = static_cast<int>(t->emIRColor);
            self->telemetry_.record_mode        = static_cast<int>(t->emRecordMode);
            self->telemetry_.valid = true;

            snapshot = self->telemetry_;
        }
        self->telemetry_cv_.notify_all();

        // 通知用户回调 (在 mutex 外调用, 避免死锁)
        TelemetryCallback cb;
        {
            std::lock_guard<std::mutex> lock(self->mutex_);
            cb = self->telemetry_cb_;
        }
        if (cb) {
            cb(snapshot);
        }
    }

    return 0;
}

}  // namespace viewlink
