#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "FlyControlCodec.h"
#include "FlyControlFrame.h"
#include "FlyControlProtocol.h"
#include "MySerial.h"

// =============================================================================
// MyFlyControl — 飞控通信业务层
//
// 功能概述：
//   1. 通过 JSON 配置初始化串口连接
//   2. 启动后台接收线程，持续从串口读取数据并拆帧
//   3. 提供各类飞行控制指令的发送接口
//   4. 维护最新的飞控心跳状态，线程安全可读
//   5. 支持注册回调：心跳更新、指令回复、云台控制
//   6. 指令发送支持超时等待回复（2秒超时）
//
// 使用示例：
//   fly_control::MyFlyControl fc;
//   fc.Init(json_config);
//   fc.Start();
//   fc.SetOnHeartbeat([](const HeartbeatData& hb){ ... });
//   fc.SendSetSpeed(1500);  // 速度 15.00 m/s
//   auto status = fc.GetLatestHeartbeat();
//   fc.Stop();
// =============================================================================

namespace fly_control {

// 回调类型定义
using HeartbeatCallback     = std::function<void(const HeartbeatData&)>;
using CommandReplyCallback  = std::function<void(const CommandReplyData&)>;
using GimbalControlCallback = std::function<void(const GimbalControlData&)>;

class MyFlyControl {
public:
    MyFlyControl();
    ~MyFlyControl();

    // -----------------------------------------------------------------------
    // 初始化与生命周期
    // -----------------------------------------------------------------------

    // 使用 JSON 配置初始化（包含串口配置）
    // 配置示例：{ "port": "/dev/ttyS1", "baudrate": 115200, "timeout_ms": 100 }
    bool Init(const nlohmann::json& cfg, std::string* err = nullptr);

    // 启动后台接收线程
    bool Start(std::string* err = nullptr);

    // 停止后台接收线程并关闭串口
    void Stop();

    // 是否正在运行
    bool IsRunning() const;

    // -----------------------------------------------------------------------
    // 回调注册
    // -----------------------------------------------------------------------

    // 注册心跳更新回调（每次收到心跳帧时调用）
    void SetOnHeartbeat(HeartbeatCallback cb);

    // 注册指令回复回调（每次收到回复帧时调用）
    void SetOnCommandReply(CommandReplyCallback cb);

    // 注册云台控制回调（每次收到云台指令时调用）
    void SetOnGimbalControl(GimbalControlCallback cb);

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    // 获取最新的心跳数据（线程安全）
    HeartbeatData GetLatestHeartbeat() const;

    // 获取最新心跳的 JSON 表示
    nlohmann::json GetLatestHeartbeatJson() const;

    // 获取故障信息解析结果
    FaultBits GetFaultBits() const;

    // 是否收到过心跳
    bool HasHeartbeat() const;

    // -----------------------------------------------------------------------
    // 发送控制指令（域控→飞控）
    // -----------------------------------------------------------------------

    // 设置飞行目的地
    bool SendSetDestination(int32_t lon, int32_t lat, uint16_t alt, std::string* err = nullptr);

    // 设置飞行航线
    bool SendSetRoute(const std::vector<RoutePoint>& points, std::string* err = nullptr);

    // 设置角度
    bool SendSetAngle(int16_t pitch, uint16_t yaw, std::string* err = nullptr);

    // 设置飞行速度
    bool SendSetSpeed(uint16_t speed, std::string* err = nullptr);

    // 设置飞行高度
    bool SendSetAltitude(uint8_t alt_type, uint16_t altitude, std::string* err = nullptr);

    // 电源开关控制
    bool SendPowerSwitch(uint8_t command, std::string* err = nullptr);

    // 开伞控制
    bool SendParachute(uint8_t parachute_type, std::string* err = nullptr);

    // 指令按钮
    bool SendButtonCommand(uint8_t button, std::string* err = nullptr);

    // 设置原点/返航点
    bool SendSetOriginReturn(uint8_t point_type, int32_t lon, int32_t lat,
                             uint16_t alt, std::string* err = nullptr);

    // 设置电子围栏
    bool SendSetGeofence(const std::vector<GeofencePoint>& points, std::string* err = nullptr);

    // 切换运行模式
    bool SendSwitchMode(uint8_t mode, std::string* err = nullptr);

    // 发送末制导指令
    bool SendGuidance(const GuidanceData& data, std::string* err = nullptr);

    // 发送末制导指令新版
    bool SendGuidanceNew(const GuidanceNewData& data, std::string* err = nullptr);

    // 发送吊舱姿态-角速度模式
    bool SendGimbalAngRate(const GimbalAngRateData& data, std::string* err = nullptr);

    // 发送吊舱姿态-角度模式
    bool SendGimbalAngle(const GimbalAngleData& data, std::string* err = nullptr);

    // 发送识别目标状态
    bool SendTargetState(const TargetStateData& data, std::string* err = nullptr);

private:
    // 后台接收线程主循环
    void ReceiveLoop();

    // 处理一个已解析的帧
    void HandleFrame(const ParsedFrame& frame);

    // 通过串口发送字节数据
    bool SendRawData(const std::vector<uint8_t>& data, std::string* err);

    // 获取下一个 CNT 值（自增计数器）
    uint8_t NextCnt();

    // 将 HeartbeatData 转为 JSON
    static nlohmann::json HeartbeatToJson(const HeartbeatData& hb);

private:
    my_serial::MySerial           serial_;         // 串口实例
    FrameParser                   parser_;         // 帧解析器
    std::atomic<bool>             running_{false}; // 运行标志
    std::unique_ptr<std::thread>  recv_thread_;    // 接收线程

    mutable std::mutex            hb_mutex_;       // 心跳数据锁
    HeartbeatData                 latest_hb_;      // 最新心跳数据
    bool                          has_hb_{false};  // 是否收到过心跳

    std::mutex                    send_mutex_;     // 发送锁
    std::atomic<uint8_t>          cnt_{0};         // 帧计数器

    // 回调函数
    HeartbeatCallback             on_heartbeat_;
    CommandReplyCallback          on_reply_;
    GimbalControlCallback         on_gimbal_;
    std::mutex                    cb_mutex_;       // 回调锁
};

} // namespace fly_control
