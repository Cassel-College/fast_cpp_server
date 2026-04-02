#pragma once

/**
 * @file BaseAudio.h
 * @brief 扬声器框架 —— 音频设备基类
 *
 * 定义了所有音频设备必须实现的纯虚接口，以及通用的状态/配置数据结构。
 * 厂商适配层需继承此基类并实现全部纯虚方法。
 */

#include <string>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

namespace my_audio {

// ============================================================================
// 枚举定义
// ============================================================================

/**
 * @brief 音频设备状态枚举
 */
enum class AudioStatus {
    Working,    ///< 正在工作（播放中）
    Idle,       ///< 空闲待命
    Offline,    ///< 离线（未连接）
    Error       ///< 故障状态
};

/**
 * @brief 将 AudioStatus 转为可读中文字符串
 */
inline std::string AudioStatusToString(AudioStatus status) {
    switch (status) {
        case AudioStatus::Working:  return "工作中";
        case AudioStatus::Idle:     return "空闲";
        case AudioStatus::Offline:  return "离线";
        case AudioStatus::Error:    return "故障";
        default:                    return "未知";
    }
}

// ============================================================================
// 配置与信息结构体
// ============================================================================

/**
 * @brief 音频设备初始化配置
 */
struct AudioConfig {
    std::string device_id;          ///< 设备唯一标识
    std::string name;               ///< 设备名称
    std::string type;               ///< 设备类型（厂商标识，如 "audio_server"）
    std::string ip;                 ///< 设备 IP 地址
    int         port = 0;           ///< 设备端口
    int         default_volume = 50;///< 默认音量（0-100）

    /**
     * @brief 从 JSON 对象解析配置
     * @param j JSON 对象，字段参见 config.json 中 audio_args
     */
    static AudioConfig FromJson(const nlohmann::json& j) {
        AudioConfig cfg;
        if (j.contains("device_id")) {
            // device_id 可能为整数或字符串，统一转成字符串
            if (j["device_id"].is_number()) {
                cfg.device_id = std::to_string(j["device_id"].get<int>());
            } else {
                cfg.device_id = j["device_id"].get<std::string>();
            }
        }
        if (j.contains("name"))           cfg.name           = j["name"].get<std::string>();
        if (j.contains("type"))           cfg.type           = j["type"].get<std::string>();
        if (j.contains("ip"))             cfg.ip             = j["ip"].get<std::string>();
        if (j.contains("port"))           cfg.port           = j["port"].get<int>();
        if (j.contains("default_volume")) cfg.default_volume = j["default_volume"].get<int>();
        return cfg;
    }

    /**
     * @brief 序列化为 JSON
     */
    nlohmann::json ToJson() const {
        return {
            {"device_id",      device_id},
            {"name",           name},
            {"type",           type},
            {"ip",             ip},
            {"port",           port},
            {"default_volume", default_volume}
        };
    }
};

/**
 * @brief 音频设备描述信息（型号、固件版本等）
 */
struct AudioInfo {
    std::string model;              ///< 型号
    std::string firmware_version;   ///< 固件版本
    std::string hardware_version;   ///< 硬件版本
    std::string serial_number;      ///< 序列号
    std::string ip_ping_status;     ///< IP 联通状态（在线/离线/未知）

    nlohmann::json ToJson() const {
        return {
            {"model",            model},
            {"firmware_version", firmware_version},
            {"hardware_version", hardware_version},
            {"serial_number",    serial_number},
            {"ip_ping_status",   ip_ping_status}
        };
    }
};

// ============================================================================
// 音频设备纯虚基类
// ============================================================================

/**
 * @brief 音频设备纯虚基类
 *
 * 所有厂商适配层需继承此类并实现全部纯虚接口。
 * 基类提供通用属性管理与线程安全的状态访问。
 */
class BaseAudio {
public:
    BaseAudio() = default;
    virtual ~BaseAudio() = default;

    // 禁止拷贝
    BaseAudio(const BaseAudio&) = delete;
    BaseAudio& operator=(const BaseAudio&) = delete;

    // ========================================================================
    // 纯虚接口 —— 厂商必须实现
    // ========================================================================

    /**
     * @brief 初始化设备
     * @param json_config JSON 格式的配置字符串
     * @return true 初始化成功
     */
    virtual bool Init(const std::string& json_config) = 0;

    /**
     * @brief 启动设备（开始监听/服务）
     * @return true 启动成功
     */
    virtual bool Start() = 0;

    /**
     * @brief 停止设备
     * @return true 停止成功
     */
    virtual bool Stop() = 0;

    /**
     * @brief 获取当前设备状态
     */
    virtual AudioStatus Status() = 0;

    /**
     * @brief 获取初始化配置
     */
    virtual AudioConfig Config() = 0;

    /**
     * @brief 获取设备描述信息（型号、固件版本等）
     */
    virtual AudioInfo Info() = 0;

    /**
     * @brief 播放指定路径的音频文件
     * @param path 音频文件路径
     * @return true 播放命令发送成功
     */
    virtual bool PlayFile(const std::string& path) = 0;

    /**
     * @brief 设置音量
     * @param volume 音量值（0-100），默认 50
     * @return true 设置成功
     */
    virtual bool SetVolume(int volume = 50) = 0;

    /**
     * @brief 设备自检
     * @return true 自检通过
     */
    virtual bool CheckSelf() = 0;

    /**
     * @brief 心跳与属性定时更新循环
     *
     * 此方法应在独立线程中运行，定期更新设备状态和属性。
     * 当 IsStopRequested() 返回 true 时应退出循环。
     */
    virtual void AudioLoop() = 0;

    // ========================================================================
    // 通用方法
    // ========================================================================

    /** @brief 获取设备 ID */
    std::string GetDeviceId() const { return device_id_; }

    /** @brief 获取设备名称 */
    std::string GetName() const { return name_; }

    /** @brief 请求停止 AudioLoop */
    void RequestStop() { stop_flag_.store(true); }

    /** @brief 检查是否已请求停止 */
    bool IsStopRequested() const { return stop_flag_.load(); }

protected:
    std::string              device_id_;                            ///< 设备唯一标识
    std::string              name_;                                 ///< 设备名称
    std::atomic<int>         volume_{50};                           ///< 当前音量
    std::atomic<AudioStatus> status_{AudioStatus::Offline};         ///< 当前状态
    std::atomic<bool>        stop_flag_{false};                     ///< 停止标志
    mutable std::mutex       mutex_;                                ///< 通用互斥锁
};

} // namespace my_audio
