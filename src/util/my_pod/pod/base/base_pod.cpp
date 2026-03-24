/**
 * @file base_pod.cpp
 * @brief 基础吊舱实现
 */

#include "base_pod.h"
#include "MyLog.h"

namespace PodModule {

BasePod::BasePod(const PodInfo& info) : pod_info_(info) {
    MYLOG_INFO("[吊舱] BasePod 构造: id={}, name={}, vendor={}",
               info.pod_id, info.pod_name, podVendorToString(info.vendor));
}

BasePod::~BasePod() {
    monitor_.stop();
    MYLOG_INFO("[吊舱] BasePod 析构: {}", pod_info_.pod_id);
}

// ==================== 设备基础信息 ====================

PodInfo BasePod::getPodInfo() const { return pod_info_; }
std::string BasePod::getPodId() const { return pod_info_.pod_id; }
std::string BasePod::getPodName() const { return pod_info_.pod_name; }
PodVendor BasePod::getVendor() const { return pod_info_.vendor; }

// ==================== 连接管理 ====================

PodResult<void> BasePod::connect() {
    if (state_ == PodState::CONNECTED) {
        return PodResult<void>::fail(PodErrorCode::ALREADY_CONNECTED);
    }
    if (!session_) {
        return PodResult<void>::fail(PodErrorCode::SESSION_NOT_INITIALIZED);
    }

    setState(PodState::CONNECTING);
    auto result = session_->connect(pod_info_.ip_address, pod_info_.port);
    if (result.isSuccess()) {
        setState(PodState::CONNECTED);
    } else {
        setState(PodState::ERROR);
    }
    return result;
}

PodResult<void> BasePod::disconnect() {
    if (state_ == PodState::DISCONNECTED) {
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED);
    }
    PodResult<void> result = PodResult<void>::success();
    if (session_ && session_->isConnected()) {
        result = session_->disconnect();
    }
    setState(PodState::DISCONNECTED);
    return result;
}

bool BasePod::isConnected() const { return state_ == PodState::CONNECTED; }
PodState BasePod::getState() const { return state_; }

// ==================== Session 管理 ====================

void BasePod::setSession(std::shared_ptr<ISession> session) {
    session_ = session;
}

std::shared_ptr<ISession> BasePod::getSession() const { return session_; }

// ==================== 配置 ====================

void BasePod::setCapabilityConfigs(const nlohmann::json& capability_section) {
    capability_configs_ = capability_section;
}

nlohmann::json BasePod::getCapabilityConfig(CapabilityType type) const {
    auto key = capabilityTypeToString(type);
    if (capability_configs_.contains(key) && capability_configs_[key].is_object()) {
        return capability_configs_[key];
    }
    return nlohmann::json::object();
}

// ==================== 内部方法 ====================

void BasePod::setState(PodState state) {
    state_ = state;
}

// ==================== 后台监控 ====================

PodRuntimeStatus BasePod::getRuntimeStatus() const { return monitor_.getRuntimeStatus(); }

void BasePod::startMonitor(const PodMonitorConfig& config) {
    MYLOG_INFO("[吊舱] {} 启动后台监控", pod_info_.pod_id);
    monitor_.start(this, config);
}

void BasePod::stopMonitor() {
    MYLOG_INFO("[吊舱] {} 停止后台监控", pod_info_.pod_id);
    monitor_.stop();
}

bool BasePod::isMonitorRunning() const { return monitor_.isRunning(); }

// ==================== 状态查询（默认占位） ====================

PodResult<PodStatus> BasePod::getDeviceStatus() {
    return PodResult<PodStatus>::success(cached_status_, "获取状态成功（占位）");
}

PodResult<void> BasePod::refreshDeviceStatus() {
    return PodResult<void>::success("刷新状态成功（占位）");
}

// ==================== 心跳检测（默认占位） ====================

PodResult<bool> BasePod::sendHeartbeat() {
    alive_ = true;
    return PodResult<bool>::success(true, "心跳发送成功（占位）");
}

PodResult<void> BasePod::startHeartbeat(uint32_t interval_ms) {
    heartbeat_interval_ms_ = interval_ms;
    heartbeat_running_ = true;
    return PodResult<void>::success("心跳检测已启动（占位）");
}

PodResult<void> BasePod::stopHeartbeat() {
    heartbeat_running_ = false;
    return PodResult<void>::success("心跳检测已停止（占位）");
}

bool BasePod::isAlive() const { return alive_; }

// ==================== 云台控制（默认占位） ====================

PodResult<PTZPose> BasePod::getPose() {
    return PodResult<PTZPose>::success(current_pose_, "获取姿态成功（占位）");
}

PodResult<void> BasePod::setPose(const PTZPose& pose) {
    current_pose_ = pose;
    return PodResult<void>::success("设置姿态成功（占位）");
}

PodResult<void> BasePod::controlSpeed(const PTZCommand& /*cmd*/) {
    return PodResult<void>::success("速度控制成功（占位）");
}

PodResult<void> BasePod::stopPtz() {
    return PodResult<void>::success("停止成功（占位）");
}

PodResult<void> BasePod::goHome() {
    current_pose_ = PTZPose{};
    return PodResult<void>::success("回中成功（占位）");
}

// ==================== 激光测距（默认占位） ====================

PodResult<LaserInfo> BasePod::laserMeasure() {
    if (!laser_enabled_) {
        return PodResult<LaserInfo>::fail(PodErrorCode::LASER_MEASURE_FAILED, "激光器未开启");
    }
    LaserInfo info;
    info.is_valid = true;
    info.distance = 100.0;
    last_laser_measurement_ = info;
    return PodResult<LaserInfo>::success(info, "测距成功（占位）");
}

PodResult<void> BasePod::enableLaser() {
    laser_enabled_ = true;
    return PodResult<void>::success("激光器已开启（占位）");
}

PodResult<void> BasePod::disableLaser() {
    laser_enabled_ = false;
    return PodResult<void>::success("激光器已关闭（占位）");
}

bool BasePod::isLaserEnabled() const { return laser_enabled_; }

// ==================== 流媒体（默认占位） ====================

PodResult<StreamInfo> BasePod::startStream(StreamType type) {
    if (streaming_) {
        return PodResult<StreamInfo>::success(current_stream_, "已在拉流中");
    }
    current_stream_.type = type;
    current_stream_.url = "rtsp://127.0.0.1:8554/mock";
    current_stream_.width = 1920;
    current_stream_.height = 1080;
    current_stream_.fps = 30;
    current_stream_.is_active = true;
    streaming_ = true;
    return PodResult<StreamInfo>::success(current_stream_, "拉流启动成功（占位）");
}

PodResult<void> BasePod::stopStream() {
    if (!streaming_) {
        return PodResult<void>::fail(PodErrorCode::STREAM_STOP_FAILED, "当前未在拉流");
    }
    streaming_ = false;
    current_stream_.is_active = false;
    return PodResult<void>::success("拉流已停止（占位）");
}

PodResult<StreamInfo> BasePod::getStreamInfo() {
    return PodResult<StreamInfo>::success(current_stream_, "获取流信息成功");
}

bool BasePod::isStreaming() const { return streaming_; }

// ==================== 图像抓拍（默认占位） ====================

PodResult<ImageFrame> BasePod::captureImage() {
    ImageFrame frame;
    frame.width = 1920;
    frame.height = 1080;
    frame.channels = 3;
    frame.format = "JPEG";
    return PodResult<ImageFrame>::success(frame, "抓拍成功（占位）");
}

PodResult<std::string> BasePod::saveImage(const std::string& path) {
    return PodResult<std::string>::success(path, "保存成功（占位）");
}

// ==================== 中心点测量（默认占位） ====================

PodResult<CenterMeasurementResult> BasePod::centerMeasure() {
    CenterMeasurementResult result;
    result.is_valid = true;
    result.distance = 150.0;
    result.center_x = 960.0;
    result.center_y = 540.0;
    last_center_result_ = result;
    return PodResult<CenterMeasurementResult>::success(result, "中心点测量成功（占位）");
}

PodResult<void> BasePod::startContinuousMeasure(uint32_t interval_ms) {
    measure_interval_ms_ = interval_ms;
    continuous_measure_running_ = true;
    return PodResult<void>::success("连续测量已启动（占位）");
}

PodResult<void> BasePod::stopContinuousMeasure() {
    continuous_measure_running_ = false;
    return PodResult<void>::success("连续测量已停止（占位）");
}

PodResult<CenterMeasurementResult> BasePod::getLastMeasureResult() {
    return PodResult<CenterMeasurementResult>::success(last_center_result_, "获取成功");
}

} // namespace PodModule
