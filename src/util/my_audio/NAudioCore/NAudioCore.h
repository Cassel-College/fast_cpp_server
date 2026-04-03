#pragma once

/**
 * @file NAudioCore.h
 * @brief NAudio 厂商 SDK 核心封装层
 *
 * 将 NAudioServerLib.h 中的 C 接口封装为面向对象的 C++ 类，
 * 提供服务启停、设备管理、文件播放、音量控制等基础能力。
 * 
 * 此类由 NAudio 内嵌调用，不直接暴露给上层使用者。
 */

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

// NAudio 厂商 SDK 头文件
#include "NAudioServerLib.h"

namespace my_audio {

// ============================================================================
// 回调类型定义
// ============================================================================

/**
 * @brief 设备状态变更回调
 * @param devno  设备编号
 * @param status 新状态码（DEVS_OFFLINE, DEVS_IDLE, DEVS_PLAYING 等）
 */
using DeviceStatusCallback = std::function<void(unsigned int devno, unsigned short status)>;

/**
 * @brief 播放状态变更回调
 * @param scene  播放场景
 * @param inst   播放实例
 * @param status 播放状态（PS_STOP, PS_PLAYING, PS_PAUSE）
 */
using PlayStatusCallback = std::function<void(unsigned char scene, unsigned char inst, unsigned int status)>;

// ============================================================================
// NAudioCore 类
// ============================================================================

/**
 * @brief NAudio SDK 核心封装类
 *
 * 封装厂商 C 接口为 C++ 类，管理服务生命周期、设备状态、音频播放。
 * 同一进程中只应存在一个实例（由 NAudio 管理）。
 */
class NAudioCore {
public:
    NAudioCore();
    ~NAudioCore();

    // 禁止拷贝
    NAudioCore(const NAudioCore&) = delete;
    NAudioCore& operator=(const NAudioCore&) = delete;

    // ========================================================================
    // 服务生命周期
    // ========================================================================

    /**
     * @brief 启动 NAudio 音频服务
     * @param server_ip  服务器监听 IP
     * @param port       设备命令端口
     * @return true 启动成功
     */
    bool StartServer(const std::string& server_ip, int port);

    /**
     * @brief 停止 NAudio 音频服务
     */
    void StopServer();

    /**
     * @brief 服务是否正在运行
     */
    bool IsRunning() const { return running_.load(); }

    // ========================================================================
    // 设备管理
    // ========================================================================

    /**
     * @brief 获取在线设备列表
     * @return 在线设备编号列表
     */
    std::vector<unsigned int> GetOnlineDevices();

    /**
     * @brief 获取设备状态
     * @param devno 设备编号
     * @return 设备状态码（DEVS_OFFLINE 等），失败返回 DEVS_OFFLINE
     */
    unsigned short GetDeviceStatus(unsigned int devno);

    /**
     * @brief 获取设备音量
     * @param devno 设备编号
     * @return 音量值 0-100，失败返回 -1
     */
    int GetDeviceVolume(unsigned int devno);

    /**
     * @brief 设置设备音量
     * @param devno  设备编号
     * @param volume 音量值 0-100
     * @return true 设置成功
     */
    bool SetDeviceVolume(unsigned int devno, unsigned char volume);

    /**
     * @brief 获取设备固件版本
     * @param devno 设备编号
     * @return 固件版本字符串，失败返回空
     */
    std::string GetDeviceVersion(unsigned int devno);

    /**
     * @brief 获取设备名称
     * @param devno 设备编号
     * @return 设备名称字符串
     */
    std::string GetDeviceName(unsigned int devno);

    // ========================================================================
    // 音频播放
    // ========================================================================

    /**
     * @brief 播放音频文件到指定设备
     * @param devno    目标设备编号
     * @param filepath 音频文件路径（支持 mp3/wma/wav）
     * @param volume   播放音量 0-100
     * @param priority 播放优先级 0-255
     * @return true 播放启动成功
     */
    bool PlayFileOnDevice(unsigned int devno, const std::string& filepath,
                          int volume = 80, int priority = 10);

    /**
     * @brief 停止指定场景和实例的播放
     * @param scene 播放场景
     * @param inst  播放实例
     * @return true 停止成功
     */
    bool StopPlay(unsigned char scene = SCENE_PLAY_AUDIOFILE,
                  unsigned char inst = PLAY_INST1);

    /**
     * @brief 获取播放信息
     * @param scene    播放场景
     * @param inst     播放实例
     * @param playinfo 输出播放信息
     * @return true 获取成功
     */
    bool GetPlayInfo(unsigned char scene, unsigned char inst, _play_info& playinfo);

    // ========================================================================
    // 回调设置
    // ========================================================================

    /**
     * @brief 设置设备状态变更回调（上层由 NAudio 注册）
     */
    void SetDeviceStatusCallback(DeviceStatusCallback cb);

    /**
     * @brief 设置播放状态变更回调
     */
    void SetPlayStatusCallback(PlayStatusCallback cb);

    /**
     * @brief 设置心跳超时时间
     * @param timeout_sec 超时时间（秒），默认 10
     */
    void SetHeartbeatTimeout(unsigned int timeout_sec = 10);

private:
    // SDK 回调的静态分发函数（C 接口要求函数指针）
    static void OnDeviceStatusChange(unsigned int devno, unsigned short status);
    static void OnPlayStatusChange(unsigned char scene, unsigned char inst, unsigned int status);

    std::atomic<bool>     running_{false};       ///< 服务运行标志
    mutable std::mutex    mutex_;                ///< 通用锁

    // 本实例的回调（通过静态 instance 指针分发）
    DeviceStatusCallback  device_status_cb_;
    PlayStatusCallback    play_status_cb_;

    // 静态实例指针，用于 C 回调分发到本对象
    static NAudioCore*    instance_;
    static std::mutex     instance_mutex_;
};

} // namespace my_audio
