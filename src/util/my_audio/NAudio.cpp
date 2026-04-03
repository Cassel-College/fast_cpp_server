#include "NAudio.h"
#include "MyLog.h"
#include "PingTools.h"

#include <chrono>
#include <nlohmann/json.hpp>

/**
 * @file NAudio.cpp
 * @brief 扬声器框架 —— NAudio 厂商适配层实现
 *
 * 实现 BaseAudio 接口，通过 NAudioCore 调用厂商 SDK。
 */

namespace my_audio {

// ============================================================================
// 构造/析构
// ============================================================================

NAudio::NAudio()
    : core_(std::make_unique<NAudioCore>()) {
    MYLOG_INFO("[NAudio] 厂商适配实例已创建");
}

NAudio::~NAudio() {
    Stop();
    MYLOG_INFO("[NAudio] 厂商适配实例已销毁, 设备={}", device_id_);
}

// ============================================================================
// 初始化
// ============================================================================

bool NAudio::Init(const std::string& json_config) {
    MYLOG_INFO("[NAudio] 开始初始化...");

    try {
        auto j = nlohmann::json::parse(json_config);
        config_ = AudioConfig::FromJson(j);

        // 设置基类属性
        device_id_ = config_.device_id;
        name_ = config_.name;
        volume_.store(config_.default_volume);

        // 从配置中提取 SDK 设备编号
        if (j.contains("device_id") && j["device_id"].is_number()) {
            sdk_devno_ = j["device_id"].get<unsigned int>();
        } else {
            sdk_devno_ = static_cast<unsigned int>(std::stoul(config_.device_id));
        }

        // 设置回调
        core_->SetDeviceStatusCallback(
            [this](unsigned int devno, unsigned short status) {
                OnDeviceStatusChanged(devno, status);
            });
        core_->SetPlayStatusCallback(
            [this](unsigned char scene, unsigned char inst, unsigned int status) {
                OnPlayStatusChanged(scene, inst, status);
            });

        status_.store(AudioStatus::Offline);

        info_.ip_ping_status = "离线";

        MYLOG_INFO("[NAudio] 初始化完成 -> 名称={}, ID={}, IP={}:{}, SDK设备号={}",
                   config_.name, config_.device_id, config_.ip, config_.port, sdk_devno_);
        return true;

    } catch (const std::exception& e) {
        MYLOG_ERROR("[NAudio] 初始化失败, 配置解析异常: {}", e.what());
        status_.store(AudioStatus::Error);
        return false;
    }
}

// ============================================================================
// 启动/停止
// ============================================================================

bool NAudio::Start() {
    MYLOG_INFO("[NAudio] 启动设备服务, 名称={}, IP={}:{}",
               config_.name, config_.ip, config_.port);

    if (!core_->StartServer(config_.ip, config_.port)) {
        MYLOG_ERROR("[NAudio] 启动失败, 名称={}", config_.name);
        status_.store(AudioStatus::Error);
        return false;
    }

    // 等待设备上线
    MYLOG_INFO("[NAudio] 等待设备上线（{}秒）...", 3);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 检测设备是否在线
    auto devices = core_->GetOnlineDevices();
    bool found = false;
    for (auto devno : devices) {
        if (devno == sdk_devno_) {
            found = true;
            break;
        }
    }

    if (found) {
        status_.store(AudioStatus::Idle);
        MYLOG_INFO("[NAudio] 设备 {} 已上线, 在线设备总数={}", sdk_devno_, devices.size());
    } else {
        status_.store(AudioStatus::Offline);
        MYLOG_WARN("[NAudio] 设备 {} 未在线, 在线设备数={}, 将在 AudioLoop 中持续检测",
                   sdk_devno_, devices.size());
    }

    // 获取设备信息
    info_.firmware_version = core_->GetDeviceVersion(sdk_devno_);
    info_.model = config_.type;
    info_.serial_number = std::to_string(sdk_devno_);

    // 设置默认音量
    SetVolume(config_.default_volume);

    // 在自有线程中启动 AudioLoop 心跳循环
    loop_thread_ = std::thread([this]() { AudioLoop(); });

    MYLOG_INFO("[NAudio] 设备启动完成, 名称={}, 状态={}",
               config_.name, AudioStatusToString(status_.load()));
    return true;
}

bool NAudio::Stop() {
    MYLOG_INFO("[NAudio] 停止设备, 名称={}", config_.name);

    // 请求 AudioLoop 停止
    RequestStop();

    // 等待 AudioLoop 线程退出
    if (loop_thread_.joinable()) {
        loop_thread_.join();
    }

    // 停止 SDK 服务
    core_->StopServer();
    status_.store(AudioStatus::Offline);

    MYLOG_INFO("[NAudio] 设备已停止, 名称={}", config_.name);
    return true;
}

// ============================================================================
// 状态/配置/信息
// ============================================================================

AudioStatus NAudio::Status() {
    return status_.load();
}

AudioConfig NAudio::Config() {
    return config_;
}

AudioInfo NAudio::Info() {
    return info_;
}

// ============================================================================
// 播放与音量
// ============================================================================

bool NAudio::PlayFile(const std::string& path) {
    MYLOG_INFO("[NAudio] 播放文件 -> 设备={}, 文件={}", config_.name, path);

    if (status_.load() == AudioStatus::Offline || status_.load() == AudioStatus::Error) {
        MYLOG_WARN("[NAudio] 设备 {} 当前状态={}, 无法播放",
                   config_.name, AudioStatusToString(status_.load()));
        return false;
    }

    bool ok = core_->PlayFileOnDevice(sdk_devno_, path, volume_.load());
    if (ok) {
        status_.store(AudioStatus::Working);
        MYLOG_INFO("[NAudio] 播放已启动, 设备={}, 文件={}", config_.name, path);
    }
    return ok;
}

bool NAudio::SetVolume(int volume) {
    // 限制音量范围 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    MYLOG_INFO("[NAudio] 设置音量 -> 设备={}, 音量={}", config_.name, volume);

    bool ok = core_->SetDeviceVolume(sdk_devno_, static_cast<unsigned char>(volume));
    if (ok) {
        volume_.store(volume);
    }
    return ok;
}

// ============================================================================
// 自检
// ============================================================================

bool NAudio::CheckSelf() {
    MYLOG_INFO("[NAudio] 执行自检 -> 设备={}", config_.name);

    // 检查 SDK 服务是否运行
    if (!core_->IsRunning()) {
        MYLOG_ERROR("[NAudio] 自检失败: SDK 服务未运行, 设备={}", config_.name);
        return false;
    }

    // 检查设备是否在线
    unsigned short dev_status = core_->GetDeviceStatus(sdk_devno_);
    if (dev_status == DEVS_OFFLINE) {
        MYLOG_ERROR("[NAudio] 自检失败: 设备离线, 设备={}", config_.name);
        status_.store(AudioStatus::Offline);
        return false;
    }

    // 检查音量是否可读
    int vol = core_->GetDeviceVolume(sdk_devno_);
    if (vol < 0) {
        MYLOG_WARN("[NAudio] 自检警告: 无法读取设备音量, 设备={}", config_.name);
    }

    MYLOG_INFO("[NAudio] 自检通过, 设备={}, SDK状态={}, 音量={}",
               config_.name, dev_status, vol);
    return true;
}

// ============================================================================
// 心跳循环
// ============================================================================

void NAudio::AudioLoop() {
    MYLOG_INFO("[NAudio] AudioLoop 启动, 设备={}", config_.name);

    while (!IsStopRequested()) {
        // 先检测设备 IP:Port 的基础联通性，避免 SDK 查询长时间阻塞。
        try {
            const bool reachable = my_tools::ping_tools::PingFuncBySystem::PingIP(config_.ip);

            if (!reachable) {
                info_.ip_ping_status = "离线";
            } else {
                info_.ip_ping_status = "在线";
            }
        } catch (const std::exception& e) {
            MYLOG_ERROR("[NAudio] 网络联通性检测异常: {}, 设备={}", e.what(), config_.name);
        }

        try {
            // 定期检查设备状态
            unsigned short dev_status = core_->GetDeviceStatus(sdk_devno_);
            AudioStatus new_status = ConvertDeviceStatus(dev_status);
            AudioStatus old_status = status_.load();

            if (new_status != old_status) {
                MYLOG_INFO("[NAudio] 设备状态变更: {} -> {}, 设备={}",
                           AudioStatusToString(old_status),
                           AudioStatusToString(new_status),
                           config_.name);
                status_.store(new_status);
            }

            // 更新设备信息
            info_.firmware_version = core_->GetDeviceVersion(sdk_devno_);

            // 更新音量
            int vol = core_->GetDeviceVolume(sdk_devno_);
            if (vol >= 0) {
                volume_.store(vol);
            }

        } catch (const std::exception& e) {
            MYLOG_ERROR("[NAudio] AudioLoop 异常: {}, 设备={}", e.what(), config_.name);
        }

        // 每 5 秒更新一次
        for (int i = 0; i < 5 && !IsStopRequested(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            MYLOG_INFO("[NAudio] AudioLoop 心跳, 设备ID={}, IP Ping: {} 当前状态={}, 音量={}",
                       config_.device_id,
                       info_.ip_ping_status,
                       AudioStatusToString(status_.load()),
                       volume_.load());
        }
    }

    MYLOG_INFO("[NAudio] AudioLoop 已退出, 设备={}, 设备ID={}", config_.name, config_.device_id);
}

// ============================================================================
// 内部辅助
// ============================================================================

AudioStatus NAudio::ConvertDeviceStatus(unsigned short sdk_status) {
    switch (sdk_status) {
        case DEVS_OFFLINE:
            return AudioStatus::Offline;
        case DEVS_IDLE:
            return AudioStatus::Idle;
        case DEVS_PLAYING:
        case DEVS_INTERCOM:
        case DEVS_SAMPLER:
        case DEVS_ALARM:
        case DEVS_LOCAL_PLAYING:
        case DEVS_TTS_PLAYING:
            return AudioStatus::Working;
        default:
            return AudioStatus::Error;
    }
}

void NAudio::OnDeviceStatusChanged(unsigned int devno, unsigned short status) {
    if (devno != sdk_devno_) return;

    const char* status_names[] = {
        "离线", "空闲", "播放中", "对讲中", "采播中", "报警中", "本地播放中", "TTS播放中"
    };
    const char* name = (status < 8) ? status_names[status] : "未知";
    MYLOG_INFO("[NAudio] 设备状态回调: 设备编号={}, 状态={}", devno, name);

    AudioStatus new_status = ConvertDeviceStatus(status);
    status_.store(new_status);
}

void NAudio::OnPlayStatusChanged(unsigned char scene, unsigned char inst, unsigned int status) {
    const char* scene_names[] = {"文件播放", "TTS播放", "实时寻呼", "音频流播放"};
    const char* status_names[] = {"停止", "播放中", "暂停中"};

    const char* scene_name = (scene < 4) ? scene_names[scene] : "未知";
    const char* status_name = (status < 3) ? status_names[status] : "未知";

    MYLOG_INFO("[NAudio] 播放状态回调: 场景={}, 实例={}, 状态={}, 设备={}",
               scene_name, inst + 1, status_name, config_.name);

    // 播放停止时切换回空闲
    if (status == PS_STOP) {
        status_.store(AudioStatus::Idle);
    } else if (status == PS_PLAYING) {
        status_.store(AudioStatus::Working);
    }
}

} // namespace my_audio
