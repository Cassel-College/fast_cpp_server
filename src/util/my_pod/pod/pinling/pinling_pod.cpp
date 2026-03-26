/**
 * @file pinling_pod.cpp
 * @brief 品凌吊舱实现
 */

#include "pinling_pod.h"
#include "../../session/pinling/pinling_session.h"
#include "../../../my_tools/PingTools.h"
#include "ViewLinkClient.h"
#include "MyLog.h"
#include "MyStartDir.h"
#include "MyFolderTools.h"


#include <cmath>
#include <thread>

namespace PodModule {

static constexpr int kMaxRetryCount = 3;
static constexpr int kRetryIntervalMs = 3000;
static constexpr uint64_t kReconnectCooldownMs = 5000;

namespace {

struct PinlingSdkConfig {
    std::string ip;
    uint16_t port = 0;
    std::string link_type = "tcp";
    bool has_init_args = false;
};

PodResult<PinlingSdkConfig> resolvePinlingSdkConfig(const PodInfo& pod_info,
                                                    const nlohmann::json& init_args) {
    PinlingSdkConfig config;
    config.ip = pod_info.ip_address;
    config.port = static_cast<uint16_t>(pod_info.port);

    if (init_args.is_object() && !init_args.empty()) {
        config.has_init_args = true;

        if (init_args.contains("ip") && init_args["ip"].is_string()) {
            config.ip = init_args["ip"].get<std::string>();
        }
        if (init_args.contains("port") && init_args["port"].is_number_integer()) {
            const int raw_port = init_args["port"].get<int>();
            if (raw_port <= 0 || raw_port > 65535) {
                return PodResult<PinlingSdkConfig>::fail(PodErrorCode::UNKNOWN_ERROR,
                                                         "PTZ init_args.port 超出有效范围");
            }
            config.port = static_cast<uint16_t>(raw_port);
        }
        if (init_args.contains("link_type") && init_args["link_type"].is_string()) {
            config.link_type = init_args["link_type"].get<std::string>();
        }
    }

    if (config.ip.empty()) {
        return PodResult<PinlingSdkConfig>::fail(PodErrorCode::UNKNOWN_ERROR,
                                                 "PTZ 已启用但未提供可用 IP");
    }
    if (config.port == 0) {
        return PodResult<PinlingSdkConfig>::fail(PodErrorCode::UNKNOWN_ERROR,
                                                 "PTZ 已启用但未提供可用端口");
    }

    return PodResult<PinlingSdkConfig>::success(config);
}

uint64_t steadyNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

} // namespace

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

PodResult<void> PinlingPod::Init(const nlohmann::json& pod_config) {
    MYLOG_INFO("[PINLING吊舱] {} 开始 PinlingPod::Init", pod_info_.pod_id);

    auto result = BasePod::Init(pod_config);
    if (!result.isSuccess()) {
        MYLOG_ERROR("[PINLING吊舱] {} PinlingPod::Init 失败: {}",
                    pod_info_.pod_id, result.message);
        return result;
    }

    MYLOG_INFO("[PINLING吊舱] {} PinlingPod::Init 完成", pod_info_.pod_id);
    return PodResult<void>::success("PinlingPod 初始化成功");
}

PodResult<void> PinlingPod::onInit(const nlohmann::json& /*pod_config*/) {
    if (!getSession()) {
        MYLOG_INFO("[PINLING吊舱] {} Init 阶段创建 PinlingSession", pod_info_.pod_id);
        setSession(std::make_shared<PinlingSession>());
    } else {
        MYLOG_INFO("[PINLING吊舱] {} Init 阶段复用已有 Session", pod_info_.pod_id);
    }

    if (!vlk_client_) {
        MYLOG_INFO("[PINLING吊舱] {} Init 阶段创建 ViewLinkClient", pod_info_.pod_id);
        vlk_client_ = std::make_unique<viewlink::ViewLinkClient>();
    }

    if (vlk_client_->IsConnected()) {
        MYLOG_WARN("[PINLING吊舱] {} Init 前检测到 SDK 仍连接，先断开旧连接", pod_info_.pod_id);
        disconnectSDK();
    }

    sdk_connected_.store(false);
    sdk_reconnect_enabled_.store(false);
    reconnecting_.store(false);
    last_reconnect_attempt_ms_.store(0);
    MYLOG_INFO("[PINLING吊舱] {} SDK 运行态已复位", pod_info_.pod_id);

    if (!isCapabilityEnabled(CapabilityType::PTZ)) {
        MYLOG_INFO("[PINLING吊舱] {} PTZ 能力未启用，Init 跳过 SDK 参数预校验", pod_info_.pod_id);
        return PodResult<void>::success("PTZ 未启用，跳过品凌 SDK 初始化准备");
    }

    const auto init_args = getCapabilityInitArgs(CapabilityType::PTZ);
    auto sdk_config_result = resolvePinlingSdkConfig(pod_info_, init_args);
    if (!sdk_config_result.isSuccess()) {
        MYLOG_ERROR("[PINLING吊舱] {} PTZ 初始化参数校验失败: {}",
                    pod_info_.pod_id, sdk_config_result.message);
        return PodResult<void>::fail(sdk_config_result.code, sdk_config_result.message);
    }

    const auto& sdk_config = sdk_config_result.data.value();
    MYLOG_INFO("[PINLING吊舱] {} Init 完成 PTZ 参数准备: source={}, link_type={}, ip={}, port={}",
               pod_info_.pod_id,
               sdk_config.has_init_args ? "capability.PTZ.init_args" : "pod 默认配置",
               sdk_config.link_type,
               sdk_config.ip,
               sdk_config.port);

    return PodResult<void>::success("PinlingPod 品凌特有初始化完成");
}

PodResult<void> PinlingPod::connect() {
    auto result = BasePod::connect();
    if (!result.isSuccess()) {
        return result;
    }

    if (!isCapabilityEnabled(CapabilityType::PTZ)) {
        sdk_reconnect_enabled_.store(false);
        MYLOG_INFO("[PINLING吊舱] {} PTZ 能力未启用，跳过 ViewLink SDK 初始化", pod_info_.pod_id);
        return result;
    }

    sdk_reconnect_enabled_.store(true);

    auto sdkResult = connectFromConfig();
    if (!sdkResult.isSuccess()) {
        MYLOG_ERROR("[PINLING吊舱] {} ViewLink SDK 连接失败: {}", pod_info_.pod_id, sdkResult.message);
    } else {
        MYLOG_INFO("[PINLING吊舱] {} ViewLink SDK 连接成功", pod_info_.pod_id);
    }

    return result;
}

PodResult<void> PinlingPod::disconnect() {
    sdk_reconnect_enabled_.store(false);
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
        setState(PodState::ERROR);
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, cached_status_.error_msg);
    }

    const bool is_online = my_tools::ping_tools::PingFuncBySystem::PingIP(pod_info_.ip_address, 1, 1);
    cached_status_.state = is_online ? PodState::CONNECTED : PodState::DISCONNECTED;
    cached_status_.error_msg = is_online ? "" : "Ping 探测失败，设备不可达";

    const bool sdk_link_alive = vlk_client_ && vlk_client_->IsConnected();
    sdk_connected_.store(sdk_link_alive);

    if (!is_online) {
        if (sdk_link_alive) {
            MYLOG_WARN("[PINLING吊舱] {} 设备离线，清理 ViewLink SDK 连接", pod_info_.pod_id);
        }
        disconnectSDK();
        setState(PodState::DISCONNECTED);
        return PodResult<void>::success("通过 Ping 探测判定PINLING设备离线");
    }

    if (sdk_reconnect_enabled_.load() && !sdk_connected_.load()) {
        auto reconnect_result = ensureSdkConnection("refreshDeviceStatus");
        if (reconnect_result.isSuccess()) {
            setState(PodState::CONNECTED);
        } else if (reconnecting_.load()) {
            setState(PodState::CONNECTING);
        } else {
            setState(PodState::ERROR);
        }
    } else if (sdk_connected_.load()) {
        setState(PodState::CONNECTED);
    } else {
        setState(PodState::DISCONNECTED);
    }

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
    if (!isCapabilityEnabled(CapabilityType::PTZ)) {
        return PodResult<void>::fail(PodErrorCode::CAPABILITY_NOT_SUPPORTED,
                                     "PTZ 能力未启用，跳过 ViewLink SDK 初始化");
    }

    try {
        bool delete_status = deleteSdkLogDirBeforConnect();
        if (delete_status) {
            MYLOG_INFO("[PINLING吊舱] {} SDK 日志文件夹清理成功", pod_info_.pod_id);
            MYLOG_INFO("[PINLING吊舱] {} 等待 3s 以确保文件系统完成清理", pod_info_.pod_id);
            for (int i = 0; i < 3; ++i) {
                MYLOG_INFO("[PINLING吊舱] {} 等待中... {}s", pod_info_.pod_id, i + 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        } else {
            MYLOG_WARN("[PINLING吊舱] {} SDK 日志文件夹清理失败或不存在，可能存在权限问题", pod_info_.pod_id);
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("[PINLING吊舱] {} 删除 SDK 日志文件夹失败: {}", pod_info_.pod_id, e.what());
    }
    MYLOG_INFO("[PINLING吊舱] {} 从配置连接 ViewLink SDK", pod_info_.pod_id);

    const auto ptz_cfg = getCapabilityConfig(CapabilityType::PTZ);
    const auto init_args = getCapabilityInitArgs(CapabilityType::PTZ);

    auto sdk_config_result = resolvePinlingSdkConfig(pod_info_, init_args);
    if (!sdk_config_result.isSuccess()) {
        MYLOG_ERROR("[PINLING吊舱] {} PTZ 初始化参数无效: {}",
                    pod_info_.pod_id, sdk_config_result.message);
        return PodResult<void>::fail(sdk_config_result.code, sdk_config_result.message);
    }

    const auto& sdk_config = sdk_config_result.data.value();

    if (sdk_config.has_init_args) {
        MYLOG_INFO("[PINLING吊舱] {} 从配置读取 PTZ init_args: link_type={}, ip={}, port={}",
                   pod_info_.pod_id, sdk_config.link_type, sdk_config.ip, sdk_config.port);
    } else {
        MYLOG_INFO("[PINLING吊舱] {} PTZ 已启用但未配置 init_args，使用 Pod 默认: ip={}, port={}",
                   pod_info_.pod_id, sdk_config.ip, sdk_config.port);
    }

    if (!ptz_cfg.empty()) {
        MYLOG_INFO("[PINLING吊舱] {} PTZ 配置: open={}, description={}",
                   pod_info_.pod_id,
                   ptz_cfg.value("open", std::string("<unset>")),
                   ptz_cfg.value("description", std::string("")));
    }

    if (sdk_config.link_type != "tcp") {
        MYLOG_WARN("[PINLING吊舱] {} 当前仅支持 TCP，配置 link_type={} 将按 tcp 处理",
                   pod_info_.pod_id, sdk_config.link_type);
    }

    PodResult<void> result;
    for (int attempt = 1; attempt <= kMaxRetryCount; ++attempt) {
        result = connectSDK(sdk_config.ip, sdk_config.port);
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
    if (vlk_client_ && vlk_client_->IsConnected()) {
        sdk_connected_.store(true);
        return PodResult<void>::success("ViewLink SDK 已连接，跳过重复连接");
    }
    sdk_connected_.store(false);

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

    sdk_connected_.store(vlk_client_->IsConnected());
    MYLOG_INFO("[PINLING吊舱] {} ViewLink SDK 连接成功 ({}:{})", pod_info_.pod_id, ip, port);
    return PodResult<void>::success("ViewLink SDK 连接成功");
}

void PinlingPod::disconnectSDK() {
    if (!vlk_client_) {
        sdk_connected_.store(false);
        return;
    }
    if (!sdk_connected_.load() && !vlk_client_->IsConnected()) {
        sdk_connected_.store(false);
        return;
    }
    MYLOG_INFO("[PINLING吊舱] {} 正在断开 ViewLink SDK ...", pod_info_.pod_id);
    vlk_client_->Shutdown();
    sdk_connected_.store(false);
}

bool PinlingPod::isSdkConnected() const {
    const bool connected = vlk_client_ && vlk_client_->IsConnected();
    return connected;
}

PodResult<void> PinlingPod::ensureSdkConnection(const char* trigger) {
    if (!isCapabilityEnabled(CapabilityType::PTZ)) {
        return PodResult<void>::fail(PodErrorCode::CAPABILITY_NOT_SUPPORTED,
                                     "PTZ 能力未启用");
    }

    const bool connected = vlk_client_ && vlk_client_->IsConnected();
    sdk_connected_.store(connected);
    if (connected) {
        return PodResult<void>::success("ViewLink SDK 已连接");
    }

    if (cached_status_.state != PodState::CONNECTED) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "吊舱当前离线，跳过 SDK 重连");
    }

    if (!sdk_reconnect_enabled_.load()) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 自动重连未启用");
    }

    const uint64_t now_ms = steadyNowMs();
    const uint64_t last_ms = last_reconnect_attempt_ms_.load();
    if (last_ms != 0 && now_ms - last_ms < kReconnectCooldownMs) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 重连冷却中");
    }

    bool expected = false;
    if (!reconnecting_.compare_exchange_strong(expected, true)) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "ViewLink SDK 重连进行中");
    }

    last_reconnect_attempt_ms_.store(now_ms);
    MYLOG_WARN("[PINLING吊舱] {} 检测到 SDK 断连，触发重连: {}", pod_info_.pod_id, trigger);

    auto reconnect_result = connectFromConfig();
    sdk_connected_.store(vlk_client_ && vlk_client_->IsConnected());
    reconnecting_.store(false);

    if (!reconnect_result.isSuccess()) {
        MYLOG_WARN("[PINLING吊舱] {} SDK 重连失败: {}", pod_info_.pod_id, reconnect_result.message);
        return reconnect_result;
    }

    MYLOG_INFO("[PINLING吊舱] {} SDK 重连成功: {}", pod_info_.pod_id, trigger);
    return reconnect_result;
}

// ============================================================================
// 云台控制（ViewLink SDK）
// ============================================================================

PodResult<PTZPose> PinlingPod::getPose() {
    auto ensure_result = ensureSdkConnection("getPose");
    if (!ensure_result.isSuccess()) {
        return PodResult<PTZPose>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
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
    auto ensure_result = ensureSdkConnection("setPose");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
    }
    vlk_client_->TurnTo(pose.yaw, pose.pitch);
    if (pose.zoom > 0 && std::abs(pose.zoom - current_pose_.zoom) > 0.01) {
        vlk_client_->ZoomTo(static_cast<float>(pose.zoom));
    }
    return PodResult<void>::success("设置云台姿态指令已发送");
}

PodResult<void> PinlingPod::controlSpeed(const PTZCommand& cmd) {
    auto ensure_result = ensureSdkConnection("controlSpeed");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
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
    auto ensure_result = ensureSdkConnection("stopPtz");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
    }
    vlk_client_->StopMove();
    vlk_client_->StopZoom();
    return PodResult<void>::success("云台已停止");
}

PodResult<void> PinlingPod::goHome() {
    auto ensure_result = ensureSdkConnection("goHome");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
    }
    vlk_client_->Home();
    return PodResult<void>::success("云台回中指令已发送");
}

// ============================================================================
// 扩展控制 —— 变焦
// ============================================================================

PodResult<void> PinlingPod::zoomTo(float magnification) {
    auto ensure_result = ensureSdkConnection("zoomTo");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
    }
    vlk_client_->ZoomTo(magnification);
    return PodResult<void>::success("变焦指令已发送");
}

PodResult<void> PinlingPod::zoomSpeed(int speed) {
    auto ensure_result = ensureSdkConnection("zoomSpeed");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
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
    auto ensure_result = ensureSdkConnection("stopZoom");
    if (!ensure_result.isSuccess()) {
        return PodResult<void>::fail(
            ensure_result.code,
            ensure_result.message.empty() ? "ViewLink SDK 未连接" : ensure_result.message);
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


bool PinlingPod::deleteSdkLogDirBeforConnect() {

    bool findLogDir     = false;
    bool deleteStatus   = false;

    MYLOG_INFO("[PINLING吊舱] {} 删除 SDK 日志文件夹（如果存在）以避免权限问题", pod_info_.pod_id);
    std::string log_dir = "/var/fast_cpp_server/logs/pinling_sdk_logs";
    
    try {
        MYLOG_INFO("开始获取当前工作目录");
        my_tools::MyStartDir myStartDir;
        std::string current_dir = myStartDir.getStartDir();
        MYLOG_INFO("当前工作目录: {}", current_dir);
        MYLOG_INFO("检查当前目录下是否存在 SDK 日志文件夹...");
        std::string target_log_dir_name = "VLKLog";
        if (my_tools::MyFolderTools::HasFolder(current_dir, target_log_dir_name)) {
            log_dir = current_dir + "/" + target_log_dir_name;
            MYLOG_INFO("找到 SDK 日志文件夹: {}", log_dir);
            findLogDir = true;
        } else {
            MYLOG_WARN("当前目录下未找到 {} 文件夹，使用默认日志路径: {}", target_log_dir_name, log_dir);
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取当前工作目录失败: {}", e.what());
    }
    if (findLogDir) {
        if (std::filesystem::exists(log_dir)) {
            try {
                std::filesystem::remove_all(log_dir);
                MYLOG_INFO("[PINLING吊舱] {} SDK 日志文件夹已删除: {}", pod_info_.pod_id, log_dir);
                deleteStatus = true;
            } catch (const std::filesystem::filesystem_error& e) {
                MYLOG_ERROR("[PINLING吊舱] {} 删除 SDK 日志文件夹失败: {}, 错误信息: {}",
                            pod_info_.pod_id, log_dir, e.what());
            }
        } else {
            MYLOG_INFO("[PINLING吊舱] {} SDK 日志文件夹不存在，无需删除: {}", pod_info_.pod_id, log_dir);
            deleteStatus = true;
        }
    } else {
        MYLOG_ERROR("[PINLING吊舱] {} 未找到 SDK 日志文件夹，无法删除: {}",
                    pod_info_.pod_id, log_dir);
    }
    return deleteStatus;
}


} // namespace PodModule
