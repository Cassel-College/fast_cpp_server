/**
 * @file pinling_pod.cpp
 * @brief 品凌吊舱实现
 */

#include "pinling_pod.h"
#include "../../session/pinling/pinling_session.h"
#include "../../../my_tools/PingTools.h"
#include "ViewLinkClient.h"
#include "MyLog.h"

#include <cmath>
#include <thread>

namespace PodModule {

static constexpr int kMaxRetryCount = 3;
static constexpr int kRetryIntervalMs = 3000;

// ============================================================================
// 构造 / 析构
// ============================================================================

PinlingPod::PinlingPod(const PodInfo& info)
    : BasePod(info)
    , vlk_client_(std::make_unique<viewlink::ViewLinkClient>())
{
    MYLOG_INFO("[PINLING吊舱] PinlingPod 构造: {}", info.pod_id);
    auto session = std::make_shared<PinlingSession>();
    setSession(session);
}

PinlingPod::~PinlingPod() {
    disconnectSDK();
    MYLOG_INFO("[PINLING吊舱] PinlingPod 析构: {}", pod_info_.pod_id);
}

// ============================================================================
// 连接管理
// ============================================================================

PodResult<void> PinlingPod::connect() {
    auto result = BasePod::connect();
    if (!result.isSuccess()) {
        return result;
    }

    auto sdkResult = connectFromConfig();
    if (!sdkResult.isSuccess()) {
        MYLOG_ERROR("[PINLING吊舱] {} ViewLink SDK 连接失败: {}", pod_info_.pod_id, sdkResult.message);
    } else {
        MYLOG_INFO("[PINLING吊舱] {} ViewLink SDK 连接成功", pod_info_.pod_id);
    }

    return result;
}

PodResult<void> PinlingPod::disconnect() {
    disconnectSDK();
    return BasePod::disconnect();
}

// ============================================================================
// 状态查询（Ping 探测）
// ============================================================================

PodResult<PodStatus> PinlingPod::getDeviceStatus() {
    auto refresh_result = refreshDeviceStatus();
    if (!refresh_result.isSuccess()) {
        return PodResult<PodStatus>::fail(refresh_result.code, refresh_result.message);
    }
    const bool is_online = cached_status_.state == PodState::CONNECTED;
    return PodResult<PodStatus>::success(
        cached_status_,
        is_online ? "通过 Ping 探测判定PINLING设备在线" : "通过 Ping 探测判定PINLING设备离线");
}

PodResult<void> PinlingPod::refreshDeviceStatus() {
    if (pod_info_.ip_address.empty()) {
        cached_status_.state = PodState::ERROR;
        cached_status_.error_msg = "PINLING设备 IP 未配置";
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, cached_status_.error_msg);
    }

    const bool is_online = my_tools::ping_tools::PingFuncBySystem::PingIP(pod_info_.ip_address, 1, 1);
    cached_status_.state = is_online ? PodState::CONNECTED : PodState::DISCONNECTED;
    cached_status_.error_msg = is_online ? "" : "Ping 探测失败，设备不可达";
    return PodResult<void>::success(
        is_online ? "通过 Ping 探测判定PINLING设备在线" : "通过 Ping 探测判定PINLING设备离线");
}

// ============================================================================
// 心跳检测
// ============================================================================

PodResult<bool> PinlingPod::sendHeartbeat() {
    alive_ = true;
    return PodResult<bool>::success(true, "品凌心跳发送成功（占位）");
}

// ============================================================================
// ViewLink SDK 生命周期
// ============================================================================

PodResult<void> PinlingPod::connectFromConfig() {
    MYLOG_INFO("[PINLING吊舱] {} 从配置连接 ViewLink SDK", pod_info_.pod_id);

    auto ptz_cfg = getCapabilityConfig(CapabilityType::PTZ);

    std::string ip   = pod_info_.ip_address;
    uint16_t    port = pod_info_.port;

    if (ptz_cfg.contains("init_args") && ptz_cfg["init_args"].is_object()) {
        const auto& args = ptz_cfg["init_args"];
        if (args.contains("ip") && args["ip"].is_string()) {
            ip = args["ip"].get<std::string>();
        }
        if (args.contains("port") && args["port"].is_number_integer()) {
            port = static_cast<uint16_t>(args["port"].get<int>());
        }
        MYLOG_INFO("[PINLING吊舱] {} 从配置读取 PTZ init_args: ip={}, port={}", pod_info_.pod_id, ip, port);
    } else {
        MYLOG_INFO("[PINLING吊舱] {} PTZ 无 init_args，使用 Pod 默认: ip={}, port={}", pod_info_.pod_id, ip, port);
    }

    PodResult<void> result;
    for (int attempt = 1; attempt <= kMaxRetryCount; ++attempt) {
        result = connectSDK(ip, port);
        if (result.isSuccess()) {
            MYLOG_INFO("[PINLING吊舱] {} SDK 连接成功", pod_info_.pod_id);
            return result;
        }
        if (attempt < kMaxRetryCount) {
            MYLOG_WARN("[PINLING吊舱] {} SDK 连接失败（第 {}/{} 次），{}ms 后重试: {}",
                       pod_info_.pod_id, attempt, kMaxRetryCount, kRetryIntervalMs, result.message);
            std::this_thread::sleep_for(std::chrono::milliseconds(kRetryIntervalMs));
        }
    }
    MYLOG_ERROR("[PINLING吊舱] {} SDK 连接失败，已耗尽 {} 次重试", pod_info_.pod_id, kMaxRetryCount);
    return result;
}

PodResult<void> PinlingPod::connectSDK(const std::string& ip, uint16_t port) {
    if (sdk_connected_.load()) {
        return PodResult<void>::success("ViewLink SDK 已连接，跳过重复连接");
    }

    try {
        vlk_client_->Initialize();
    } catch (const std::exception& e) {
        return PodResult<void>::fail(PodErrorCode::CONNECTION_FAILED,
                                     std::string("SDK 初始化失败: ") + e.what());
    }

    // 注册遥测回调
    vlk_client_->SetTelemetryCallback(
        [this](const viewlink::TelemetrySnapshot& t) {
            current_pose_.yaw   = t.attitude.yaw;
            current_pose_.pitch = t.attitude.pitch;
            current_pose_.roll  = t.attitude.roll;
            current_pose_.zoom  = t.zoom_magnification;

            TelemetryCallback cb;
            {
                std::lock_guard<std::mutex> lock(telemetry_mutex_);
                cb = telemetry_cb_;
            }
            if (cb) {
                cb(current_pose_);
            }
        }
    );

    try {
        viewlink::ConnectionConfig config;
        config.type = viewlink::ConnectionType::Tcp;
        config.ip   = ip;
        config.port = static_cast<int>(port);
        vlk_client_->Connect(config);
    } catch (const std::exception& e) {
        vlk_client_->Shutdown();
        return PodResult<void>::fail(PodErrorCode::CONNECTION_FAILED,
                                     std::string("SDK 连接失败: ") + e.what());
    }

    if (!vlk_client_->WaitForConnection(std::chrono::milliseconds(10000))) {
        vlk_client_->Shutdown();
        return PodResult<void>::fail(PodErrorCode::CONNECTION_TIMEOUT,
                                     "ViewLink SDK 连接超时");
    }

    if (!vlk_client_->WaitForTelemetry(std::chrono::milliseconds(5000))) {
        MYLOG_WARN("[PINLING吊舱] {} 等待遥测数据超时（5s），云台可能暂无反馈", pod_info_.pod_id);
    }

    sdk_connected_.store(true);
    MYLOG_INFO("[PINLING吊舱] {} ViewLink SDK 连接成功 ({}:{})", pod_info_.pod_id, ip, port);
    return PodResult<void>::success("ViewLink SDK 连接成功");
}

void PinlingPod::disconnectSDK() {
    if (!sdk_connected_.load()) return;
    MYLOG_INFO("[PINLING吊舱] {} 正在断开 ViewLink SDK ...", pod_info_.pod_id);
    vlk_client_->Shutdown();
    sdk_connected_.store(false);
}

bool PinlingPod::isSdkConnected() const {
    return sdk_connected_.load();
}

// ============================================================================
// 云台控制（ViewLink SDK）
// ============================================================================

PodResult<PTZPose> PinlingPod::getPose() {
    if (!sdk_connected_.load()) {
        return PodResult<PTZPose>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK \u672a\u8fde\u63a5");
    }
    auto att = vlk_client_->GetAttitude();
    if (!att.valid) {
        return PodResult<PTZPose>::success(current_pose_, "返回缓存姿态（遥测暂不可用）");
    }
    auto telemetry = vlk_client_->GetTelemetry();
    current_pose_.yaw   = att.yaw;
    current_pose_.pitch = att.pitch;
    current_pose_.roll  = att.roll;
    current_pose_.zoom  = telemetry.zoom_magnification;
    return PodResult<PTZPose>::success(current_pose_, "获取云台姿态成功");
}

PodResult<void> PinlingPod::setPose(const PTZPose& pose) {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    vlk_client_->TurnTo(pose.yaw, pose.pitch);
    if (pose.zoom > 0 && std::abs(pose.zoom - current_pose_.zoom) > 0.01) {
        vlk_client_->ZoomTo(static_cast<float>(pose.zoom));
    }
    return PodResult<void>::success("设置云台姿态指令已发送");
}

PodResult<void> PinlingPod::controlSpeed(const PTZCommand& cmd) {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    auto h_speed = static_cast<short>(cmd.yaw_speed * 100.0);
    auto v_speed = static_cast<short>(cmd.pitch_speed * 100.0);
    vlk_client_->Move(h_speed, v_speed);

    if (std::abs(cmd.zoom_speed) > 0.01) {
        int zoom_spd = static_cast<int>(cmd.zoom_speed);
        if (zoom_spd > 0) {
            vlk_client_->ZoomIn(static_cast<short>(std::min(zoom_spd, 8)));
        } else {
            vlk_client_->ZoomOut(static_cast<short>(std::min(-zoom_spd, 8)));
        }
    }
    return PodResult<void>::success("云台速度控制指令已发送");
}

PodResult<void> PinlingPod::stopPtz() {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    vlk_client_->StopMove();
    vlk_client_->StopZoom();
    return PodResult<void>::success("云台已停止");
}

PodResult<void> PinlingPod::goHome() {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    vlk_client_->Home();
    return PodResult<void>::success("云台回中指令已发送");
}

// ============================================================================
// 扩展控制 —— 变焦
// ============================================================================

PodResult<void> PinlingPod::zoomTo(float magnification) {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    vlk_client_->ZoomTo(magnification);
    return PodResult<void>::success("变焦指令已发送");
}

PodResult<void> PinlingPod::zoomSpeed(int speed) {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    if (speed > 0) {
        vlk_client_->ZoomIn(static_cast<short>(std::min(speed, 8)));
    } else if (speed < 0) {
        vlk_client_->ZoomOut(static_cast<short>(std::min(-speed, 8)));
    } else {
        vlk_client_->StopZoom();
    }
    return PodResult<void>::success("变焦速度控制指令已发送");
}

PodResult<void> PinlingPod::stopZoom() {
    if (!sdk_connected_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 未连接");
    }
    vlk_client_->StopZoom();
    return PodResult<void>::success("变焦已停止");
}

void PinlingPod::setTelemetryCallback(TelemetryCallback cb) {
    std::lock_guard<std::mutex> lock(telemetry_mutex_);
    telemetry_cb_ = std::move(cb);
}

// ============================================================================
// 激光测距（占位）
// ============================================================================

PodResult<LaserInfo> PinlingPod::laserMeasure() {
    if (!laser_enabled_) {
        return PodResult<LaserInfo>::fail(PodErrorCode::LASER_MEASURE_FAILED, "品凌激光器未开启");
    }
    LaserInfo info;
    info.is_valid = true;
    info.distance = 180.0;
    info.latitude = 31.234567;
    info.longitude = 121.456789;
    info.altitude = 50.0;
    last_laser_measurement_ = info;
    return PodResult<LaserInfo>::success(info, "品凌激光测距成功（占位）");
}

PodResult<void> PinlingPod::enableLaser() {
    laser_enabled_ = true;
    return PodResult<void>::success("品凌激光器已开启（占位）");
}

PodResult<void> PinlingPod::disableLaser() {
    laser_enabled_ = false;
    return PodResult<void>::success("品凌激光器已关闭（占位）");
}

// ============================================================================
// 流媒体（占位）
// ============================================================================

PodResult<StreamInfo> PinlingPod::startStream(StreamType type) {
    current_stream_.type = type;
    current_stream_.url = "rtsp://192.168.1.200:554/pinling_stream";
    current_stream_.width = 1920;
    current_stream_.height = 1080;
    current_stream_.fps = 25;
    current_stream_.is_active = true;
    streaming_ = true;
    return PodResult<StreamInfo>::success(current_stream_, "品凌拉流启动成功（占位）");
}

PodResult<void> PinlingPod::stopStream() {
    streaming_ = false;
    current_stream_.is_active = false;
    return PodResult<void>::success("品凌拉流已停止（占位）");
}

// ============================================================================
// 图像抓拍（占位）
// ============================================================================

PodResult<ImageFrame> PinlingPod::captureImage() {
    ImageFrame frame;
    frame.width = 1920;
    frame.height = 1080;
    frame.channels = 3;
    frame.format = "JPEG";
    return PodResult<ImageFrame>::success(frame, "品凌图像抓拍成功（占位）");
}

PodResult<std::string> PinlingPod::saveImage(const std::string& path) {
    return PodResult<std::string>::success(path, "品凌图像保存成功（占位）");
}

// ============================================================================
// 中心点测量（占位）
// ============================================================================

PodResult<CenterMeasurementResult> PinlingPod::centerMeasure() {
    CenterMeasurementResult result;
    result.is_valid = true;
    result.distance = 200.0;
    result.latitude = 31.234567;
    result.longitude = 121.456789;
    result.altitude = 60.0;
    result.center_x = 960.0;
    result.center_y = 540.0;
    last_center_result_ = result;
    return PodResult<CenterMeasurementResult>::success(result, "品凌中心点测量成功（占位）");
}

} // namespace PodModule
