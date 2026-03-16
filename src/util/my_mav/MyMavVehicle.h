#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

// MAVSDK 头文件
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/manual_control/manual_control.h>


namespace my_edge {

// 定义状态结构体
struct MavVehicleStatus {
    bool connected = false;          // 是否在线（有心跳）
    bool armed = false;              // 是否解锁
    std::string flight_mode = "N/A"; // 飞行模式
    float battery_voltage = 0.0f;    // 电压
    float heading = 0.0f;            // 航向
    uint64_t system_id = 0;          // MAVLink 系统 ID
};

class MyMavVehicle {
public:
    /**
     * @brief 构造函数
     * @param name 实例名称，用于日志区分，例如 "MY_MAV_01"
     */
    explicit MyMavVehicle(const std::string& name);
    ~MyMavVehicle();

    /**
     * @brief 初始化并开始监听连接
     * @param connection_url 连接字符串，例如 "udp://0.0.0.0:14550"
     * @return true 成功启动监听
     */
    bool Init(const std::string& connection_url);

    /**
     * @brief 停止连接和线程
     */
    void Stop();

    // --------------------------------------------------------
    // 控制接口
    // --------------------------------------------------------

    // 获取当前完整状态
    MavVehicleStatus GetStatus() const;

    // 是否已连接（有心跳）
    bool IsConnected() const;

    // 解锁 (Arm)
    bool Arm();

    // 上锁 (Disarm)
    bool Disarm();

    // 切换模式 (STABILIZE, MANUAL, ALT_HOLD 等)
    bool SetMode(const std::string& mode_name);

    /**
     * @brief 手动控制输入
     * @param x 前后 [-1.0, 1.0]
     * @param y 左右 [-1.0, 1.0]
     * @param z 升沉 [-1.0, 1.0] (注意：ArduSub通常 0 是停止，负是下降，正是上升，具体视固件设置)
     * @param r 偏航 [-1.0, 1.0]
     */
    bool ManualControl(float x, float y, float z, float r);

private:
    // 内部逻辑
    void MonitorThreadFunc();                                   // 监控链路状态
    void OnNewSystem(std::shared_ptr<mavsdk::System> system);   // 发现新系统回调
    void InitPlugins();                                         // 初始化插件
    void CleanupSystem();                                       // 清理断开的系统资源

private:
    std::string name_;                                          // 实例名称
    std::string connection_url_;                                // 连接地址
    mavsdk::Mavsdk mavsdk_;                                     // MAVSDK 核心对象 (每个实例独立拥有一个 Mavsdk 上下文)
    std::shared_ptr<mavsdk::System> system_;
    std::shared_ptr<mavsdk::Action> action_;                    // 插件对象
    std::shared_ptr<mavsdk::Telemetry> telemetry_;              // 插件对象
    std::shared_ptr<mavsdk::ManualControl> manual_control_;     // 插件对象
    std::thread monitor_thread_;                                // 监控线程
    std::atomic<bool> should_exit_{false};
    mutable std::mutex data_mutex_;                             // 保护状态数据的读写
    // 缓存的状态数据
    struct {
        bool armed = false;
        std::string mode = "UNKNOWN";
        float voltage = 0.0f;
        float heading = 0.0f;
        uint64_t sys_id = 0;
    } cached_state_;

}; // class MyMavVehicle

} // namespace my_edge