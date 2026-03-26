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

// ==================== 生命周期 ====================

PodResult<void> BasePod::Init(const nlohmann::json& pod_config) {
    if (!pod_config.is_object()) {
        MYLOG_ERROR("[吊舱] {} 初始化失败：pod_config 不是 object", pod_info_.pod_id);
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, "pod_config 不是有效的 object");
    }

    if (isConnected()) {
        MYLOG_ERROR("[吊舱] {} 初始化失败：实例已连接，禁止在连接态重复 Init", pod_info_.pod_id);
        return PodResult<void>::fail(PodErrorCode::ALREADY_CONNECTED, "请先断开连接后再重新初始化");
    }

    if (isMonitorRunning()) {
        MYLOG_WARN("[吊舱] {} Init 前检测到监控线程仍在运行，先停止监控", pod_info_.pod_id);
        stopMonitor();
    }

    MYLOG_INFO("[吊舱] {} 开始 BasePod::Init，配置={}", pod_info_.pod_id, pod_config.dump());

    // 保留完整原始配置，后续能力读取与调试日志都以这份配置为最终依据。
    pod_info_.raw_config = pod_config;

    // 用配置中的最新字段覆盖运行期元信息，避免 createPod 时的初始快照与实际配置脱节。
    if (pod_config.contains("name") && pod_config["name"].is_string()) {
        pod_info_.pod_name = pod_config["name"].get<std::string>();
    }
    if (pod_config.contains("model") && pod_config["model"].is_string()) {
        pod_info_.model = pod_config["model"].get<std::string>();
    }
    if (pod_config.contains("firmware_ver") && pod_config["firmware_ver"].is_string()) {
        pod_info_.firmware_ver = pod_config["firmware_ver"].get<std::string>();
    }
    if (pod_config.contains("ip") && pod_config["ip"].is_string()) {
        pod_info_.ip_address = pod_config["ip"].get<std::string>();
    }
    if (pod_config.contains("port") && pod_config["port"].is_number_integer()) {
        pod_info_.port = pod_config["port"].get<int>();
    }

    // capability 节点是后续能力开关、init_args 和监控推导的公共输入，统一在这里收口。
    if (pod_config.contains("capability") && pod_config["capability"].is_object()) {
        setCapabilityConfigs(pod_config["capability"]);
    } else {
        capability_configs_ = nlohmann::json::object();
        MYLOG_INFO("[吊舱] {} 未配置 capability 节点，按空能力配置处理", pod_info_.pod_id);
    }

    // Init 必须把对象带回一个干净的初始态，避免上一次运行遗留的缓存影响本次行为。
    resetRuntimeState();

    // 厂商特有的补充初始化统一通过钩子扩展，避免再次把逻辑散落回外层管理器。
    auto init_result = onInit(pod_config);
    if (!init_result.isSuccess()) {
        setState(PodState::ERROR);
        MYLOG_ERROR("[吊舱] {} BasePod::Init 失败: {}", pod_info_.pod_id, init_result.message);
        return init_result;
    }

    MYLOG_INFO("[吊舱] {} BasePod::Init 完成", pod_info_.pod_id);
    return PodResult<void>::success("BasePod 初始化成功");
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
    if (!capability_section.is_object()) {
        MYLOG_WARN("[吊舱] {} 注入 capability 配置失败：配置不是 object", pod_info_.pod_id);
        return;
    }

    MYLOG_INFO("[吊舱] {} 注入 capability 配置成功，keys={}",
               pod_info_.pod_id, capability_section.dump());
}

nlohmann::json BasePod::getCapabilityConfig(CapabilityType type) const {
    const auto readConfig = [&](const nlohmann::json& source) -> nlohmann::json {
        if (!source.is_object()) {
            return nlohmann::json::object();
        }

        const auto key = capabilityTypeToKey(type);
        if (source.contains(key) && source[key].is_object()) {
            return source[key];
        }

        const auto legacy_key = capabilityTypeToString(type);
        if (source.contains(legacy_key) && source[legacy_key].is_object()) {
            return source[legacy_key];
        }

        return nlohmann::json::object();
    };

    auto config = readConfig(capability_configs_);
    if (!config.empty()) {
        MYLOG_INFO("[吊舱] {} 能力配置命中: capability={}, source=capability_configs_",
                   pod_info_.pod_id, capabilityTypeToKey(type));
        return config;
    }

    if (pod_info_.raw_config.contains("capability") && pod_info_.raw_config["capability"].is_object()) {
        auto raw_config = readConfig(pod_info_.raw_config["capability"]);
        if (!raw_config.empty()) {
            MYLOG_INFO("[吊舱] {} 能力配置命中: capability={}, source=pod_info_.raw_config.capability",
                       pod_info_.pod_id, capabilityTypeToKey(type));
            return raw_config;
        }
    }

    MYLOG_WARN("[吊舱] {} 未找到能力配置: capability={}",
               pod_info_.pod_id, capabilityTypeToKey(type));

    return nlohmann::json::object();
}

bool BasePod::isCapabilityEnabled(CapabilityType type) const {
    const auto config = getCapabilityConfig(type);
    if (config.empty()) {
        MYLOG_INFO("[吊舱] {} 能力启用状态: capability={}, enabled=true (默认)",
                   pod_info_.pod_id, capabilityTypeToKey(type));
        return true;
    }
    if (config.contains("open") && config["open"].is_string()) {
        const bool enabled = config["open"].get<std::string>() == "enable";
        MYLOG_INFO("[吊舱] {} 能力启用状态: capability={}, open={}, enabled={}",
                   pod_info_.pod_id,
                   capabilityTypeToKey(type),
                   config["open"].get<std::string>(),
                   enabled ? "true" : "false");
        return enabled;
    }

    MYLOG_INFO("[吊舱] {} 能力启用状态: capability={}, enabled=true (未配置 open，默认)",
               pod_info_.pod_id, capabilityTypeToKey(type));
    return true;
}

nlohmann::json BasePod::getCapabilityInitArgs(CapabilityType type) const {
    const auto config = getCapabilityConfig(type);
    if (config.contains("init_args") && config["init_args"].is_object()) {
        MYLOG_INFO("[吊舱] {} 读取 init_args: capability={}, args={}",
                   pod_info_.pod_id,
                   capabilityTypeToKey(type),
                   config["init_args"].dump());
        return config["init_args"];
    }

    MYLOG_INFO("[吊舱] {} 未配置 init_args: capability={}",
               pod_info_.pod_id, capabilityTypeToKey(type));
    return nlohmann::json::object();
}

PodResult<void> BasePod::onInit(const nlohmann::json& /*pod_config*/) {
    return PodResult<void>::success();
}

void BasePod::resetRuntimeState() {
    setState(PodState::DISCONNECTED);

    cached_status_ = PodStatus{};
    alive_.store(false);
    heartbeat_running_.store(false);
    heartbeat_interval_ms_ = 1000;

    current_pose_ = PTZPose{};

    laser_enabled_.store(false);
    last_laser_measurement_ = LaserInfo{};

    streaming_.store(false);
    current_stream_ = StreamInfo{};

    last_center_result_ = CenterMeasurementResult{};
    continuous_measure_running_.store(false);
    measure_interval_ms_ = 500;

    MYLOG_INFO("[吊舱] {} 运行期缓存已复位", pod_info_.pod_id);
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
