/**
 * @file RtspRelayMonitorManager.cpp
 * @brief RTSP 转发守护模块 —— 单例管理器实现
 */

#include "RtspRelayMonitorManager.h"
#include "MyLog.h"

namespace my_mediamtx_monitor {

// ============================================================================
//  单例：static local variable（C++11 线程安全）
// ============================================================================

RtspRelayMonitorManager& RtspRelayMonitorManager::GetInstance() {
    static RtspRelayMonitorManager instance;
    return instance;
}

RtspRelayMonitorManager::~RtspRelayMonitorManager() {
    Stop();
    MYLOG_INFO("[RtspRelayMonitorManager] 单例已销毁");
}

// ============================================================================
//  委托接口 —— 透传到内部 RtspRelayMonitor
// ============================================================================

bool RtspRelayMonitorManager::Init(const nlohmann::json& json_config) {
    MYLOG_INFO("[RtspRelayMonitorManager] 初始化...");
    return monitor_.Init(json_config);
}

void RtspRelayMonitorManager::Start() {
    MYLOG_INFO("[RtspRelayMonitorManager] 启动监控...");
    monitor_.Start();
}

void RtspRelayMonitorManager::Stop() {
    MYLOG_INFO("[RtspRelayMonitorManager] 停止监控...");
    monitor_.Stop();
}

RtspRelayState RtspRelayMonitorManager::GetState() const {
    return monitor_.GetState();
}

nlohmann::json RtspRelayMonitorManager::GetHeartbeat() const {
    return monitor_.GetHeartbeat();
}

nlohmann::json RtspRelayMonitorManager::GetInfo() const {
    return monitor_.GetInfo();
}

bool RtspRelayMonitorManager::IsRunning() const {
    return monitor_.GetState() == RtspRelayState::RUNNING ||
           monitor_.GetState() == RtspRelayState::STARTING;
}

} // namespace my_mediamtx_monitor
