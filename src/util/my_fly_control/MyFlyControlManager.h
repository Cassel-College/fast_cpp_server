#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "MyFlyControl.h"

// =============================================================================
// MyFlyControlManager — 飞控管理器（单例包装层）
//
// 设计目的：
//   1. 为上层业务提供全局唯一入口，避免在业务代码中到处传递 MyFlyControl 对象
//   2. 保留 MyFlyControl 自身的可实例化能力，不把底层通信对象强行改造成单例
//   3. 提供统一的生命周期管理：Init / Start / Stop / Shutdown
//   4. 在 Shutdown 时重建底层对象，清空旧回调和旧状态，方便重初始化与测试
//
// 使用建议：
//   - 应用层优先使用 MyFlyControlManager::GetInstance()
//   - 若未来存在多飞控、多串口、多实例需求，直接继续使用 MyFlyControl 即可
// =============================================================================

namespace fly_control {

class MyFlyControlManager {
public:
    // 获取管理器单例
    static MyFlyControlManager& GetInstance();

    MyFlyControlManager(const MyFlyControlManager&) = delete;
    MyFlyControlManager& operator=(const MyFlyControlManager&) = delete;

    // -----------------------------------------------------------------------
    // 生命周期管理
    // -----------------------------------------------------------------------

    // 初始化底层飞控对象
    bool Init(const nlohmann::json& cfg, std::string* err = nullptr);

    // 启动底层飞控对象的接收线程
    bool Start(std::string* err = nullptr);

    // 停止底层飞控对象
    void Stop();

    // 完整关闭管理器，并重建一个全新的底层飞控对象
    void Shutdown();

    // 是否已经完成成功初始化
    bool IsInitialized() const;

    // 是否正在运行
    bool IsRunning() const;

    // -----------------------------------------------------------------------
    // 回调注册
    // -----------------------------------------------------------------------

    void SetOnHeartbeat(HeartbeatCallback cb);
    void SetOnCommandReply(CommandReplyCallback cb);
    void SetOnGimbalControl(GimbalControlCallback cb);

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    HeartbeatData GetLatestHeartbeat() const;
    nlohmann::json GetLatestHeartbeatJson() const;
    FaultBits GetFaultBits() const;
    bool HasHeartbeat() const;

    // -----------------------------------------------------------------------
    // 指令发送（对 MyFlyControl 的统一转发）
    // -----------------------------------------------------------------------

    bool SendSetDestination(int32_t lon, int32_t lat, uint16_t alt, std::string* err = nullptr);
    bool SendSetRoute(const std::vector<RoutePoint>& points, std::string* err = nullptr);
    bool SendSetAngle(int16_t pitch, uint16_t yaw, std::string* err = nullptr);
    bool SendSetSpeed(uint16_t speed, std::string* err = nullptr);
    bool SendSetAltitude(uint8_t alt_type, uint16_t altitude, std::string* err = nullptr);
    bool SendPowerSwitch(uint8_t command, std::string* err = nullptr);
    bool SendParachute(uint8_t parachute_type, std::string* err = nullptr);
    bool SendButtonCommand(uint8_t button, std::string* err = nullptr);
    bool SendSetOriginReturn(uint8_t point_type, int32_t lon, int32_t lat,
                             uint16_t alt, std::string* err = nullptr);
    bool SendSetGeofence(const std::vector<GeofencePoint>& points, std::string* err = nullptr);
    bool SendSwitchMode(uint8_t mode, std::string* err = nullptr);
    bool SendGuidance(const GuidanceData& data, std::string* err = nullptr);
    bool SendGuidanceNew(const GuidanceNewData& data, std::string* err = nullptr);
    bool SendGimbalAngRate(const GimbalAngRateData& data, std::string* err = nullptr);
    bool SendGimbalAngle(const GimbalAngleData& data, std::string* err = nullptr);
    bool SendTargetState(const TargetStateData& data, std::string* err = nullptr);

private:
    MyFlyControlManager();
    ~MyFlyControlManager();

    // 获取当前底层飞控对象快照，保证调用期间对象生命周期有效
    std::shared_ptr<MyFlyControl> GetControllerSnapshot() const;

    // 获取已经完成初始化的底层飞控对象；若尚未初始化则返回空指针并填充错误信息
    std::shared_ptr<MyFlyControl> GetInitializedController(std::string* err) const;

private:
    mutable std::mutex            mutex_;
    std::shared_ptr<MyFlyControl> controller_;
    bool                          initialized_{false};
};

} // namespace fly_control