#pragma once

/**
 * @file base_pod.h
 * @brief 基础吊舱实现
 *
 * BasePod 是所有厂商 Pod 的基类：
 * - 保存设备元信息（PodInfo）
 * - 保存当前设备状态（PodState）
 * - 持有 Session 会话对象
 * - 提供统一 Init 生命周期，集中完成配置注入与运行态复位
 * - 提供基础 connect / disconnect 逻辑
 * - 为所有能力方法提供默认占位实现
 * - 厂商 Pod 按需覆盖具体能力方法
 */

#include "../interface/i_pod.h"
#include "../../session/interface/i_session.h"
#include "../../monitor/pod_monitor.h"
#include <memory>
#include <atomic>

namespace PodModule {

class BasePod : public IPod {
public:
    explicit BasePod(const PodInfo& info);
    ~BasePod() override;

    // ==================== 生命周期 ====================

    /**
     * @brief 使用完整的 pod 配置初始化当前实例
     *
     * 这个函数的目标是把此前分散在 PodManager、BasePod、厂商 Pod 之间的
     * “通用初始化动作”收敛到同一个入口，形成稳定的生命周期：
     * 构造 -> Init -> connect -> startMonitor。
     *
     * BasePod::Init 只处理“所有 Pod 都共用”的初始化内容：
     * - 保存完整原始配置，便于后续能力配置与 init_args 查询
     * - 用配置中的最新字段回填 pod_info_，保证运行期元信息一致
     * - 提取 capability 节点并缓存，避免外部继续手工调用 setCapabilityConfigs
     * - 复位各类运行期缓存，确保旧状态不会污染新的初始化过程
     * - 调用 onInit() 钩子，把厂商特有的补充初始化留给子类扩展
     *
     * 该函数不负责真正建立设备连接；连接动作仍由 connect() 执行。
     * 这样可以避免“初始化配置”和“建立通信链路”两个职责耦合在一起。
     *
     * @param pod_config 单个吊舱的完整 JSON 配置
     * @return 初始化结果。失败时不会继续进入 connect 阶段。
     */
    PodResult<void> Init(const nlohmann::json& pod_config) override;

    // ==================== 设备基础信息 ====================

    PodInfo getPodInfo() const override;
    std::string getPodId() const override;
    std::string getPodName() const override;
    PodVendor getVendor() const override;

    // ==================== 连接管理 ====================

    PodResult<void> connect() override;
    PodResult<void> disconnect() override;
    bool isConnected() const override;
    PodState getState() const override;

    // ==================== Session 管理 ====================

    void setSession(std::shared_ptr<ISession> session) override;
    std::shared_ptr<ISession> getSession() const override;

    // ==================== 配置 ====================

    void setCapabilityConfigs(const nlohmann::json& capability_section) override;
    nlohmann::json getCapabilityConfig(CapabilityType type) const override;

    // ==================== 后台监控 ====================

    PodRuntimeStatus getRuntimeStatus() const override;
    void startMonitor(const PodMonitorConfig& config = {}) override;
    void stopMonitor() override;
    bool isMonitorRunning() const override;

    // ==================== 状态查询（默认占位） ====================

    PodResult<PodStatus> getDeviceStatus() override;
    PodResult<void> refreshDeviceStatus() override;

    // ==================== 心跳检测（默认占位） ====================

    PodResult<bool> sendHeartbeat() override;
    PodResult<void> startHeartbeat(uint32_t interval_ms = 1000) override;
    PodResult<void> stopHeartbeat() override;
    bool isAlive() const override;

    // ==================== 云台控制（默认占位） ====================

    PodResult<PTZPose> getPose() override;
    PodResult<void> setPose(const PTZPose& pose) override;
    PodResult<void> controlSpeed(const PTZCommand& cmd) override;
    PodResult<void> stopPtz() override;
    PodResult<void> goHome() override;

    // ==================== 激光测距（默认占位） ====================

    PodResult<LaserInfo> laserMeasure() override;
    PodResult<void> enableLaser() override;
    PodResult<void> disableLaser() override;
    bool isLaserEnabled() const override;

    // ==================== 流媒体（默认占位） ====================

    PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) override;
    PodResult<void> stopStream() override;
    PodResult<StreamInfo> getStreamInfo() override;
    bool isStreaming() const override;

    // ==================== 图像抓拍（默认占位） ====================

    PodResult<ImageFrame> captureImage() override;
    PodResult<std::string> saveImage(const std::string& path) override;

    // ==================== 中心点测量（默认占位） ====================

    PodResult<CenterMeasurementResult> centerMeasure() override;
    PodResult<void> startContinuousMeasure(uint32_t interval_ms = 500) override;
    PodResult<void> stopContinuousMeasure() override;
    PodResult<CenterMeasurementResult> getLastMeasureResult() override;

protected:
    void setState(PodState state);
    bool isCapabilityEnabled(CapabilityType type) const;
    nlohmann::json getCapabilityInitArgs(CapabilityType type) const;

    /**
     * @brief 子类初始化扩展点
     *
     * BasePod::Init 完成通用初始化后，会调用该钩子。
     * 子类如需根据完整配置补充厂商特有的成员准备、参数解析或缓存初始化，
     * 应覆盖该方法，而不是再把逻辑散落回 PodManager。
     */
    virtual PodResult<void> onInit(const nlohmann::json& pod_config);

    /**
     * @brief 复位运行期缓存数据
     *
     * Init 的语义是“把实例带回一个干净、尚未连接的起始状态”。
     * 因此这里会清空上一轮运行遗留的状态、姿态、流信息、测距结果等缓存，
     * 但不会销毁构造阶段已经注入的 session 等基础依赖。
     */
    void resetRuntimeState();

    PodInfo                         pod_info_;
    PodState                        state_ = PodState::DISCONNECTED;
    std::shared_ptr<ISession>       session_;
    PodMonitor                      monitor_;
    nlohmann::json                  capability_configs_;

    // 能力相关缓存数据（子类共享）
    PodStatus                       cached_status_;
    std::atomic<bool>               alive_{false};
    std::atomic<bool>               heartbeat_running_{false};
    uint32_t                        heartbeat_interval_ms_ = 1000;
    PTZPose                         current_pose_;
    std::atomic<bool>               laser_enabled_{false};
    LaserInfo                       last_laser_measurement_;
    std::atomic<bool>               streaming_{false};
    StreamInfo                      current_stream_;
    CenterMeasurementResult         last_center_result_;
    std::atomic<bool>               continuous_measure_running_{false};
    uint32_t                        measure_interval_ms_ = 500;
};

} // namespace PodModule
