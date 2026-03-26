#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace viewlink {

// ---------------------------------------------------------------------------
// 公共数据类型
// ---------------------------------------------------------------------------

/// 连接方式
enum class ConnectionType { Tcp, Serial };

/// 连接配置参数
struct ConnectionConfig {
    ConnectionType type        = ConnectionType::Tcp;
    std::string    ip          = "192.168.2.119";
    int            port        = 2000;
    std::string    serial_port = "/dev/ttyS0";
    int            baud_rate   = 115200;
};

/// 云台姿态 (yaw/pitch/roll)
struct Attitude {
    double yaw   = 0.0;
    double pitch  = 0.0;
    double roll   = 0.0;
    bool   valid  = false;
};

/// 遥测快照
struct TelemetrySnapshot {
    Attitude attitude;
    int    sensor_type        = 0;
    int    tracker_status     = 0;
    double target_lat         = 0.0;
    double target_lng         = 0.0;
    double target_alt         = 0.0;
    double drone_lat          = 0.0;
    double drone_lng          = 0.0;
    double drone_alt          = 0.0;
    double horizontal_fov     = 0.0;
    double vertical_fov       = 0.0;
    int    eo_digital_zoom    = 0;
    int    ir_digital_zoom    = 0;
    double zoom_magnification = 0.0;
    short  laser_distance     = 0;
    int    ir_color           = 0;
    int    record_mode        = 0;
    bool   valid              = false;
};

/// 设备型号信息
struct DeviceModelInfo {
    int         model_code = 0;
    std::string model_name;
    bool        valid = false;
};

/// 设备配置信息
struct DeviceConfigInfo {
    std::string version_no;
    std::string device_id;
    std::string serial_no;
    bool        valid = false;
};

// ---------------------------------------------------------------------------
// ViewLinkClient —— ViewLink SDK 的 C++ 封装
// ---------------------------------------------------------------------------
class ViewLinkClient {
public:
    /// 遥测数据回调类型 (可选, 每次收到新遥测时被调用)
    using TelemetryCallback = std::function<void(const TelemetrySnapshot&)>;

    ViewLinkClient();
    ~ViewLinkClient();

    ViewLinkClient(const ViewLinkClient&) = delete;
    ViewLinkClient& operator=(const ViewLinkClient&) = delete;

    // =================== 生命周期 ===========================================

    /// 初始化 SDK (自动创建 VLKLog 目录)
    void Initialize();
    /// 关闭 SDK 及后台线程
    void Shutdown();
    /// SDK 是否已初始化
    bool IsInitialized() const { return initialized_; }

    // =================== 连接管理 ===========================================

    /// 发起连接 (TCP 或 Serial)
    void Connect(const ConnectionConfig& config);
    /// 主动断开
    void Disconnect();
    /// 阻塞等待连接建立, 超时返回 false
    bool WaitForConnection(std::chrono::milliseconds timeout);
    /// 阻塞等待首次遥测数据到达
    bool WaitForTelemetry(std::chrono::milliseconds timeout);

    // =================== 连接状态查询 =======================================

    bool IsConnected()       const;
    bool IsTcpConnected()    const;
    bool IsSerialConnected() const;
    std::string GetConnectionTypeName() const;

    // =================== 吊舱姿态 / 遥测 ===================================

    /// 获取当前姿态 (yaw/pitch/roll)
    Attitude GetAttitude() const;
    /// 获取完整遥测快照
    TelemetrySnapshot GetTelemetry() const;
    /// 是否已收到过遥测数据
    bool HasTelemetry() const;

    /// 获取设备型号信息
    DeviceModelInfo  GetModelInfo()  const;
    /// 获取设备配置信息
    DeviceConfigInfo GetConfigInfo() const;
    bool HasModelInfo()  const;
    bool HasConfigInfo() const;

    /// 设置遥测回调 (可选, 线程安全)
    void SetTelemetryCallback(TelemetryCallback cb);

    // =================== 云台运动控制 =======================================

    /// 以指定速度移动 (单位: 0.01°/s, 例如 1000 = 10°/s)
    void Move(short horizontal_speed, short vertical_speed);
    /// 停止移动
    void StopMove();
    /// 回到初始位置
    void Home();
    /// 转到指定 yaw/pitch (度)
    void TurnTo(double yaw, double pitch);

    // =================== 变焦控制 ===========================================

    void ZoomIn(short speed);
    void ZoomOut(short speed);
    void StopZoom();
    /// 变焦到指定倍率
    void ZoomTo(float magnification);

    // =================== 对焦控制 ===========================================

    void FocusIn(short speed);
    void FocusOut(short speed);
    void StopFocus();
    void SetFocusMode(int mode);   // 0=auto, 1=manual
    int  GetFocusMode() const;

    // =================== 跟踪控制 ===========================================

    void StartTracking(int x, int y, int video_width, int video_height);
    void StopTracking();
    bool IsTracking() const;

    // =================== 电机 / 跟随 =======================================

    void SwitchMotor(bool on);
    bool IsMotorOn() const;
    void SetFollowMode(bool enable);
    bool IsFollowMode() const;

    // =================== 录像 / 拍照 =======================================

    void Photograph();
    void SwitchRecord(bool on);
    bool IsRecording() const;

    // =================== 激光 ===============================================

    void SwitchLaser(bool on);

    // =================== 其它 ===============================================

    std::string GetSdkVersion() const;

private:
    static int OnConnectionStatus(int status, const char* msg, int len, void* user);
    static int OnDeviceStatus(int type, const char* buf, int len, void* user);

    // ------------ 数据成员 -------------------------------------------------
    mutable std::mutex      mutex_;
    std::condition_variable connection_cv_;
    std::condition_variable telemetry_cv_;

    TelemetrySnapshot telemetry_;
    DeviceModelInfo   model_info_;
    DeviceConfigInfo  config_info_;
    TelemetryCallback telemetry_cb_;

    std::atomic<bool> connected_{false};
    std::atomic<bool> tcp_connected_{false};
    std::atomic<bool> serial_connected_{false};

    bool initialized_ = false;
};

}  // namespace viewlink
