#pragma once

/**
 * @file RtspRelayMonitorManager.h
 * @brief RTSP 转发守护模块 —— 单例管理器
 *
 * 对 RtspRelayMonitor 的单例封装，提供全局唯一的访问入口。
 * 遵循项目 static local variable 单例模式（线程安全，懒加载）。
 *
 * 使用方式：
 * @code
 *   auto& mgr = RtspRelayMonitorManager::GetInstance();
 *   mgr.Init(json_config);
 *   mgr.Start();
 *   // ...
 *   mgr.Stop();
 * @endcode
 */

#include <nlohmann/json.hpp>
#include "RtspRelayMonitor.h"

namespace my_mediamtx_monitor {

/**
 * @brief RTSP 转发守护单例管理器
 *
 * 封装 RtspRelayMonitor 实例，保证全局唯一。
 * 对外暴露与 RtspRelayMonitor 相同的 Init / Start / Stop / GetState / GetHeartbeat 接口。
 */
class RtspRelayMonitorManager {
public:
    /**
     * @brief 获取全局唯一实例（线程安全，懒加载）
     */
    static RtspRelayMonitorManager& GetInstance();

    /**
     * @brief 初始化监控模块
     * @param json_config JSON 配置（包含 mediamtx_enable、mediamtx_bin_path、check_ip、mediamtx_json_config 等）
     * @return true 成功；false 失败
     */
    bool Init(const nlohmann::json& json_config);

    /**
     * @brief 启动监控线程
     */
    void Start();

    /**
     * @brief 停止监控线程并终止 MediaMTX 进程
     */
    void Stop();

    /**
     * @brief 获取当前运行状态
     */
    RtspRelayState GetState() const;

    /**
     * @brief 获取心跳信息（JSON 格式）
     */
    nlohmann::json GetHeartbeat() const;

    /**
     * @brief 获取转发代理基本信息（在 Init 时收集）
     */
    nlohmann::json GetInfo() const;

    /**
     * @brief 判断模块是否正在运行
     */
    bool IsRunning() const;

private:
    RtspRelayMonitorManager() = default;
    ~RtspRelayMonitorManager();

    // 禁止拷贝与移动
    RtspRelayMonitorManager(const RtspRelayMonitorManager&) = delete;
    RtspRelayMonitorManager& operator=(const RtspRelayMonitorManager&) = delete;

    /// 内部持有的 RtspRelayMonitor 实例
    RtspRelayMonitor monitor_;
};

} // namespace my_mediamtx_monitor
