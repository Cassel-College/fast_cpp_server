#pragma once

/**
 * @file MyAudios.h
 * @brief 扬声器框架 —— 全局音频设备管理器（单例）
 *
 * 维护全部音频设备实例的生命周期，支持根据 JSON 配置动态创建厂商适配实例。
 * 对外提供统一的设备查询、播放控制等接口。
 */

#include "BaseAudio.h"

#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <nlohmann/json.hpp>

namespace my_audio {

/**
 * @brief 全局音频设备管理器（单例）
 *
 * 使用方式:
 * @code
 *   MyAudios::GetInstance().Init(config_json);
 *   MyAudios::GetInstance().Start();
 *   MyAudios::GetInstance().PlayOnSpeaker("3445", "/path/to/file.mp3");
 * @endcode
 */
class MyAudios {
public:
    /**
     * @brief 获取单例实例
     */
    static MyAudios& GetInstance() {
        static MyAudios instance;
        return instance;
    }

    // 禁止拷贝
    MyAudios(const MyAudios&) = delete;
    MyAudios& operator=(const MyAudios&) = delete;

    // ========================================================================
    // 生命周期管理
    // ========================================================================

    /**
     * @brief 初始化：解析 JSON 配置并创建各厂商音频设备实例
     * @param config JSON 对象（对应 config.json 中 audio_server 的 model_args）
     * @return true 初始化成功
     *
     * 配置格式示例:
     * @code
     * {
     *   "audio_count": 2,
     *   "audio_args": {
     *     "audio_001": {
     *       "name": "audio_server_001",
     *       "type": "audio_server",
     *       "ip": "192.168.1.150",
     *       "port": 8890,
     *       "device_id": 3445
     *     }
     *   }
     * }
     * @endcode
     */
    bool Init(const nlohmann::json& config);

    /**
     * @brief 启动所有已初始化的音频设备
     * @return true 至少有一个设备启动成功
     */
    bool Start();

    /**
     * @brief 停止所有音频设备并释放资源
     */
    void Stop();

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 获取所有扬声器（含离线/故障设备）
     * @return 设备ID → 设备实例 的映射
     */
    std::map<std::string, std::shared_ptr<BaseAudio>> GetAllSpeakers();

    /**
     * @brief 获取当前可用的扬声器（状态为 Idle 或 Working）
     * @return 可用设备ID列表
     */
    std::vector<std::string> GetAvailableSpeakers();

    /**
     * @brief 获取指定设备的状态
     * @param device_id 设备 ID
     * @return 设备状态枚举，设备不存在时返回 Offline
     */
    AudioStatus Status(const std::string& device_id);

    /**
     * @brief 获取指定设备的配置信息
     * @param device_id 设备 ID
     * @return 设备配置，设备不存在时返回空配置
     */
    AudioConfig Config(const std::string& device_id);

    /**
     * @brief 获取指定设备的描述信息
     * @param device_id 设备 ID
     * @return 设备信息，设备不存在时返回空信息
     */
    AudioInfo Info(const std::string& device_id);

    /**
     * @brief 获取全部设备的综合状态 JSON
     */
    nlohmann::json StatusAll();

    /**
     * @brief 获取初始化时使用的配置快照
     * @return 初始化配置 JSON，未初始化时返回空对象
     */
    nlohmann::json GetInitConfig() const;

    // ========================================================================
    // 播放控制
    // ========================================================================

    /**
     * @brief 在指定扬声器上播放音频文件
     * @param device_id 设备 ID
     * @param filepath  音频文件路径
     * @return true 播放命令发送成功
     */
    bool PlayOnSpeaker(const std::string& device_id, const std::string& filepath);

    /**
     * @brief 在所有可用扬声器上播放音频文件
     * @param filepath 音频文件路径
     * @return 成功播放的设备数量
     */
    int PlayOnAllSpeakers(const std::string& filepath);

    /**
     * @brief 设置指定设备的音量
     * @param device_id 设备 ID
     * @param volume    音量值 0-100
     * @return true 设置成功
     */
    bool SetVolume(const std::string& device_id, int volume);

private:
    MyAudios() = default;
    ~MyAudios();

    /**
     * @brief 根据类型字符串创建对应厂商的音频设备实例
     * @param type 设备类型（如 "audio_server"）
     * @return 设备实例，不支持的类型返回 nullptr
     */
    std::shared_ptr<BaseAudio> CreateAudioDevice(const std::string& type);

    std::map<std::string, std::shared_ptr<BaseAudio>> speakers_;    ///< 设备实例映射
    mutable std::mutex                                mutex_;       ///< 线程安全锁
    bool                                              initialized_{false};
    nlohmann::json                                    init_config_; ///< 初始化配置快照
};

} // namespace my_audio
