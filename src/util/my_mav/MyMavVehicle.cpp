#include "MyMavVehicle.h"
#include "MyLog.h" // 用户提供的日志库

#include <chrono>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>


namespace my_edge {

using namespace mavsdk;
using namespace std::chrono_literals;

MyMavVehicle::MyMavVehicle(const std::string& name) 
    : name_(name), 
      mavsdk_(mavsdk::Mavsdk::Configuration{mavsdk::ComponentType::GroundStation}) 
{
}

MyMavVehicle::~MyMavVehicle() {
    Stop();
}

bool MyMavVehicle::Init(const std::string& connection_url) {
    connection_url_ = connection_url;

    MYLOG_INFO("[{}] 初始化连接监听: {}", name_, connection_url_);

    // 建立连接监听
    ConnectionResult ret = mavsdk_.add_any_connection(connection_url);
    if (ret != ConnectionResult::Success) {
        MYLOG_INFO("[{}] 连接监听启动失败: {}", name_, int(ret));
        return false;
    }

    // 订阅新系统发现事件
    mavsdk_.subscribe_on_new_system([this]() {
        // 由于可能存在多系统，我们简单起见，连接发现的第一个系统
        // 或者可以在这里判断 system_id 是否符合预期
        auto systems = mavsdk_.systems();
        if (!systems.empty()) {
            // 如果当前没有持有系统，则绑定新发现的系统
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (!system_) {
                OnNewSystem(systems[0]);
            }
        }
    });

    // 启动监控线程
    should_exit_ = false;
    monitor_thread_ = std::thread(&MyMavVehicle::MonitorThreadFunc, this);

    return true;
}

void MyMavVehicle::Stop() {
    should_exit_ = true;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    CleanupSystem();
}

void MyMavVehicle::OnNewSystem(std::shared_ptr<mavsdk::System> system) {
    system_ = system;
    
    // 更新缓存 ID
    cached_state_.sys_id = system_->get_system_id();
    MYLOG_INFO("[{}] 捕获到潜航器系统, ID: {}", name_, cached_state_.sys_id);

    InitPlugins();
}

void MyMavVehicle::InitPlugins() {
    if (!system_) return;

    telemetry_ = std::make_shared<mavsdk::Telemetry>(system_);
    action_ = std::make_shared<mavsdk::Action>(system_);
    manual_control_ = std::make_shared<mavsdk::ManualControl>(system_);

    // 1. Telemetry 插件
    telemetry_ = std::make_shared<Telemetry>(system_);
    
    // 订阅电池
    telemetry_->subscribe_battery([this](Telemetry::Battery battery) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        cached_state_.voltage = battery.voltage_v;
    });

    // 订阅姿态
    telemetry_->subscribe_heading([this](Telemetry::Heading heading) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        cached_state_.heading = heading.heading_deg;
    });

    // 订阅飞行模式
    telemetry_->subscribe_flight_mode([this](Telemetry::FlightMode mode) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        switch (mode) {
            case Telemetry::FlightMode::Manual: cached_state_.mode = "MANUAL"; break;
            case Telemetry::FlightMode::Stabilized: cached_state_.mode = "STABILIZE"; break;
            case Telemetry::FlightMode::Hold: cached_state_.mode = "ALT_HOLD"; break;
            default: cached_state_.mode = "UNKNOWN"; break;
        }
    });
    
    // 订阅解锁状态
    telemetry_->subscribe_armed([this](bool is_armed) {
         std::lock_guard<std::mutex> lock(data_mutex_);
         cached_state_.armed = is_armed;
    });

    // 2. Action 插件
    action_ = std::make_shared<mavsdk::Action>(system_);

    // 3. ManualControl 插件
    manual_control_ = std::make_shared<mavsdk::ManualControl>(system_);
    auto mc_ret = manual_control_->start_position_control();
    if (mc_ret != mavsdk::ManualControl::Result::Success) {
        MYLOG_INFO("[{}] 警告: 手动控制流启动失败: {}", name_, int(mc_ret));
    } else {
        MYLOG_INFO("[{}] 手动控制流已就绪", name_);
    }

    MYLOG_INFO("[{}] 插件实例化完成", name_);
}

void MyMavVehicle::CleanupSystem() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    telemetry_              = nullptr;          // 释放插件
    action_                 = nullptr;          // 释放插件
    manual_control_         = nullptr;          // 释放插件
    system_                 = nullptr;          // 释放系统引用，允许 MAVSDK 发现新系统
    cached_state_.armed     = false;            // 重置状态
    cached_state_.mode      = "DISCONNECTED";   // 重置状态
    cached_state_.sys_id    = 0;                // 重置状态
}

void MyMavVehicle::MonitorThreadFunc() {
    MYLOG_INFO("[{}] 链路监控线程启动", name_);

    while (!should_exit_) {
        bool is_link_lost = false;

        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (system_) {
                if (system_->is_connected()) {
                    // 连接正常
                } else {
                    // 发生了断链（心跳丢失）
                    MYLOG_INFO("[{}] 错误: 潜航器心跳丢失！正在尝试重连...", name_);
                    is_link_lost = true;
                }
            } else {
                // 等待连接建立中
                 MYLOG_INFO("Waiting for link {} heartbeat...", connection_url_);
            }
        }

        if (is_link_lost) {
            // 执行清理，使得 subscribe_on_new_system 可以再次触发
            CleanupSystem();
            // 注意：mavsdk_.add_any_connection 不需要重新调用，它会持续监听端口
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

// ---------------- 外部接口实现 ----------------

MavVehicleStatus MyMavVehicle::GetStatus() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    MavVehicleStatus status;
    status.connected = (system_ && system_->is_connected());
    status.armed = cached_state_.armed;
    status.flight_mode = cached_state_.mode;
    status.battery_voltage = cached_state_.voltage;
    status.heading = cached_state_.heading;
    status.system_id = cached_state_.sys_id;
    return status;
}

bool MyMavVehicle::IsConnected() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return (system_ && system_->is_connected());
}

bool MyMavVehicle::Arm() {
    // 这里的 action_ 是 shared_ptr，我们需要确保它是有效且线程安全的
    // 简单起见，复制指针进行操作
    auto act = action_; 
    if (!IsConnected() || !act) {
        MYLOG_INFO("[{}] Arm 失败: 未连接", name_);
        return false;
    }

    MYLOG_INFO("[{}] 发送解锁指令...", name_);
    Action::Result ret = act->arm();
    if (ret != Action::Result::Success) {
        MYLOG_INFO("[{}] 解锁失败: {}", name_, int(ret));
        return false;
    }
    return true;
}

bool MyMavVehicle::Disarm() {
    auto act = action_;
    if (!IsConnected() || !act) return false;

    MYLOG_INFO("[{}] 发送上锁指令...", name_);
    Action::Result ret = act->disarm();
    if (ret != Action::Result::Success) {
        MYLOG_INFO("[{}] 上锁失败: {}", name_, int(ret));
        return false;
    }
    return true;
}

bool my_edge::MyMavVehicle::SetMode(const std::string& mode_name) {
    if (!IsConnected() || !action_) return false;

    // 1. 优先尝试使用 Action 插件提供的语义化接口
    if (mode_name == "ALT_HOLD") {
        MYLOG_INFO("[{}] 切换至定深模式 (Hold)...", name_);
        return (action_->hold() == mavsdk::Action::Result::Success);
    }

    // 2. 如果 Action 不支持通用 set_flight_mode，
    // 我们手动发送 MAVLink 模式切换指令（对标 pymavlink 的实现）
    auto passthrough = std::make_shared<mavsdk::MavlinkPassthrough>(system_);
    
    // 这里需要根据 ArduSub 的模式定义 ID
    // STABILIZE 通常是 0, MANUAL 通常是 19 (具体查阅 ArduSub 官方文档)
    uint8_t base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    uint32_t custom_mode = 0; 

    if (mode_name == "STABILIZE") custom_mode = 0; 
    else if (mode_name == "MANUAL") custom_mode = 19; 

    MYLOG_INFO("[{}] 通过 Passthrough 发送模式切换: {}", name_, mode_name);
    
    // 发送 COMMAND_LONG 消息
    passthrough->send_command_long(
        mavsdk::MavlinkPassthrough::CommandLong{
            .target_sysid = passthrough->get_target_sysid(),
            .target_compid = passthrough->get_target_compid(),
            .command = MAV_CMD_DO_SET_MODE,
            .param1 = static_cast<float>(base_mode),
            .param2 = static_cast<float>(custom_mode),
            .param3 = 0.0f, .param4 = 0.0f, .param5 = 0.0f, .param6 = 0.0f, .param7 = 0.0f
        }
    );

    return true;
}

// bool MyMavVehicle::SetMode(const std::string& mode_name) {

//     if (!IsConnected() || !action_) {
//         MYLOG_ERROR("[{}] SetMode 失败: 未连接", name_);
//         return false;
//     }
//     auto act = action_;

//     // V2/V3 中 FlightMode 定义在 Telemetry 类中
//     mavsdk::Telemetry::FlightMode target_mode = mavsdk::Telemetry::FlightMode::Unknown;

//     if (mode_name == "STABILIZE") target_mode = mavsdk::Telemetry::FlightMode::Stabilized;
//     else if (mode_name == "MANUAL") target_mode = mavsdk::Telemetry::FlightMode::Manual;
//     else if (mode_name == "ALT_HOLD") target_mode = mavsdk::Telemetry::FlightMode::Hold;

//     if (target_mode == mavsdk::Telemetry::FlightMode::Unknown) {
//         MYLOG_INFO("[{}] 未知模式: {}", name_, mode_name);
//         return false;
//     }

//     // 注意：在某些 V3 版本中，设置模式可能需要通过 action_->set_flight_mode
//     // 如果报错找不到 set_flight_mode，请确认是否正确链接了插件库
//     // auto ret = action_->set_flight_mode(target_mode); 
//     // if (ret != mavsdk::Action::Result::Success) {
//     //     MYLOG_INFO("[{}] 切模式失败: {}", name_, (int)ret);
//     //     return false;
//     // }
//     // return true;
    
//     if (mavsdk::Telemetry::FlightMode::Hold == target_mode) {
//         action_->hold();                                                                // 对应潜航器的定深/悬停
//     } else if (mavsdk::Telemetry::FlightMode::Manual == target_mode) {
//         action_->transition_to_multicopter();                                           // 对应潜航器的手动模式
//     } else if (mavsdk::Telemetry::FlightMode::Stabilized == target_mode) {
//         action_->transition_to_multicopter();                                           // 对应潜航器的定姿态模式（如果固件支持）
//     } else {
//         MYLOG_INFO("[{}] 模式切换失败: 不支持的模式 {}", name_, mode_name);
//         return false;
//     }
// }

bool MyMavVehicle::ManualControl(float x, float y, float z, float r) {
    auto mc = manual_control_;
    if (!IsConnected() || !mc) return false;

    // 直接透传控制量
    ManualControl::Result ret = mc->set_manual_control_input(x, y, z, r);
    return (ret == ManualControl::Result::Success);
}; // 注意：ArduSub 的手动控制通常是 0 停止，负数下降，正数上升，具体视固件设置而定

} // namespace my_edge