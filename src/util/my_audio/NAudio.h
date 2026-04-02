#pragma once

/**
 * @file NAudio.h
 * @brief 扬声器框架 —— NAudio 厂商适配层
 *
 * 继承 BaseAudio 基类，对接 NAudio 厂商 SDK。
 * 内嵌 NAudioCore 完成底层 SDK 调用。
 *
 * 每个 NAudio 实例对应配置中的一个音频服务器节点。
 */

#include "BaseAudio.h"
#include "NAudioCore/NAudioCore.h"

#include <thread>
#include <memory>

namespace my_audio {

/**
 * @brief NAudio 厂商适配类
 *
 * 实现 BaseAudio 定义的全部纯虚接口，
 * 通过内嵌 NAudioCore 完成与厂商 SDK 的交互。
 */
class NAudio : public BaseAudio {
public:
    NAudio();
    ~NAudio() override;

    // ========================================================================
    // BaseAudio 接口实现
    // ========================================================================

    bool Init(const std::string& json_config) override;
    bool Start() override;
    bool Stop() override;

    AudioStatus Status() override;
    AudioConfig Config() override;
    AudioInfo   Info() override;

    bool PlayFile(const std::string& path) override;
    bool SetVolume(int volume = 50) override;
    bool CheckSelf() override;
    void AudioLoop() override;

private:
    /**
     * @brief 将 NAudio SDK 设备状态码转为 AudioStatus
     */
    AudioStatus ConvertDeviceStatus(unsigned short sdk_status);

    /**
     * @brief SDK 设备状态回调处理
     */
    void OnDeviceStatusChanged(unsigned int devno, unsigned short status);

    /**
     * @brief SDK 播放状态回调处理
     */
    void OnPlayStatusChanged(unsigned char scene, unsigned char inst, unsigned int status);

    AudioConfig                     config_;        ///< 设备配置
    AudioInfo                       info_;          ///< 设备信息
    std::unique_ptr<NAudioCore>     core_;          ///< SDK 核心封装
    std::thread                     loop_thread_;   ///< AudioLoop 线程
    unsigned int                    sdk_devno_{0};  ///< SDK 内部设备编号
};

} // namespace my_audio
