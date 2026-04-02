#include "NAudioCore.h"
#include "MyLog.h"

/**
 * @file NAudioCore.cpp
 * @brief NAudio 厂商 SDK 核心封装层实现
 *
 * 将 NAudioServerLib 的 C 接口封装为 C++ 类实现，
 * 通过静态回调分发机制将 SDK 回调转发给实例方法。
 */

namespace my_audio {

// ============================================================================
// 静态成员初始化
// ============================================================================
NAudioCore* NAudioCore::instance_ = nullptr;
std::mutex  NAudioCore::instance_mutex_;

// ============================================================================
// 构造/析构
// ============================================================================

NAudioCore::NAudioCore() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_ = this;
    MYLOG_INFO("[NAudioCore] 核心实例已创建");
}

NAudioCore::~NAudioCore() {
    if (running_.load()) {
        StopServer();
    }
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == this) {
        instance_ = nullptr;
    }
    MYLOG_INFO("[NAudioCore] 核心实例已销毁");
}

// ============================================================================
// 服务生命周期
// ============================================================================

bool NAudioCore::StartServer(const std::string& server_ip, int port) {
    if (running_.load()) {
        MYLOG_WARN("[NAudioCore] 服务已在运行中，IP={}, 端口={}", server_ip, port);
        return true;
    }

    MYLOG_INFO("[NAudioCore] 正在启动音频服务, IP={}, 端口={}", server_ip, port);

    int ret = na_server_start(server_ip.c_str(), port);
    if (ret == RET_START_CMDSERVER_FAIL) {
        MYLOG_ERROR("[NAudioCore] 启动命令服务失败！IP={}, 端口={}", server_ip, port);
        return false;
    } else if (ret == RET_START_DATASERVER_FAIL) {
        MYLOG_ERROR("[NAudioCore] 启动数据服务失败！IP={}, 端口={}", server_ip, port);
        return false;
    }

    // 注册 SDK 回调
    na_set_callback(OnDeviceStatusChange, OnPlayStatusChange);

    // 设置默认心跳超时
    SetHeartbeatTimeout(10);

    // 设置默认最大码率
    na_set_max_bitrate(BR_64KBPS);

    running_.store(true);
    MYLOG_INFO("[NAudioCore] 音频服务启动成功, IP={}, 端口={}", server_ip, port);
    return true;
}

void NAudioCore::StopServer() {
    if (!running_.load()) {
        return;
    }

    MYLOG_INFO("[NAudioCore] 正在停止音频服务...");
    na_server_stop();
    running_.store(false);
    MYLOG_INFO("[NAudioCore] 音频服务已停止");
}

// ============================================================================
// 设备管理
// ============================================================================

std::vector<unsigned int> NAudioCore::GetOnlineDevices() {
    unsigned int devlist[512];
    int devnbr = 512;

    na_get_online_device(devlist, &devnbr);

    std::vector<unsigned int> result(devlist, devlist + devnbr);
    MYLOG_DEBUG("[NAudioCore] 获取在线设备列表, 设备数量={}", devnbr);
    return result;
}

unsigned short NAudioCore::GetDeviceStatus(unsigned int devno) {
    unsigned short status = DEVS_OFFLINE;
    int ret = na_get_device_status(devno, &status);
    if (ret != RET_SUCCESS) {
        MYLOG_WARN("[NAudioCore] 获取设备 {} 状态失败, 返回码={}", devno, ret);
        return DEVS_OFFLINE;
    }
    return status;
}

int NAudioCore::GetDeviceVolume(unsigned int devno) {
    unsigned char volume = 0;
    int ret = na_get_device_volume(devno, &volume);
    if (ret != RET_SUCCESS) {
        MYLOG_WARN("[NAudioCore] 获取设备 {} 音量失败, 返回码={}", devno, ret);
        return -1;
    }
    return static_cast<int>(volume);
}

bool NAudioCore::SetDeviceVolume(unsigned int devno, unsigned char volume) {
    int ret = na_set_device_volume(devno, volume);
    if (ret != RET_SUCCESS) {
        MYLOG_ERROR("[NAudioCore] 设置设备 {} 音量为 {} 失败, 返回码={}", devno, volume, ret);
        return false;
    }
    MYLOG_INFO("[NAudioCore] 设备 {} 音量已设置为 {}", devno, volume);
    return true;
}

std::string NAudioCore::GetDeviceVersion(unsigned int devno) {
    char version[16] = {0};
    int ret = na_get_device_version(devno, version);
    if (ret != RET_SUCCESS) {
        MYLOG_WARN("[NAudioCore] 获取设备 {} 固件版本失败, 返回码={}", devno, ret);
        return "";
    }
    return std::string(version);
}

std::string NAudioCore::GetDeviceName(unsigned int devno) {
    char devname[32] = {0};
    int ret = na_get_device_name(devno, devname);
    if (ret != RET_SUCCESS) {
        return "";
    }
    return std::string(devname);
}

// ============================================================================
// 音频播放
// ============================================================================

bool NAudioCore::PlayFileOnDevice(unsigned int devno, const std::string& filepath,
                                   int volume, int priority) {
    MYLOG_INFO("[NAudioCore] 准备在设备 {} 上播放文件: {}", devno, filepath);

    // 设置播放音量
    int ret = na_set_play_volume(SCENE_PLAY_AUDIOFILE, PLAY_INST1, volume);
    if (ret != RET_SUCCESS) {
        MYLOG_ERROR("[NAudioCore] 设置播放音量失败, 设备={}, 音量={}, 返回码={}", devno, volume, ret);
        return false;
    }

    // 清空并添加播放设备
    na_clear_play_device(SCENE_PLAY_AUDIOFILE, PLAY_INST1);
    na_add_play_device(SCENE_PLAY_AUDIOFILE, PLAY_INST1, devno);

    // 启动播放
    ret = na_start_play_1(SCENE_PLAY_AUDIOFILE, PLAY_INST1, filepath.c_str(), priority);
    if (ret == RET_OPEN_AUDIOFILE_FAIL) {
        MYLOG_ERROR("[NAudioCore] 音频文件不可访问: {}", filepath);
        return false;
    } else if (ret == RET_AUDIO_FILE_NOT_SUPPORTED) {
        MYLOG_ERROR("[NAudioCore] 不支持的音频文件格式: {}", filepath);
        return false;
    } else if (ret != RET_SUCCESS) {
        MYLOG_ERROR("[NAudioCore] 启动播放失败, 设备={}, 返回码={}", devno, ret);
        return false;
    }

    MYLOG_INFO("[NAudioCore] 播放已启动, 设备={}, 文件={}", devno, filepath);
    return true;
}

bool NAudioCore::StopPlay(unsigned char scene, unsigned char inst) {
    int ret = na_stop_play(scene, inst);
    if (ret != RET_SUCCESS) {
        MYLOG_WARN("[NAudioCore] 停止播放失败, 场景={}, 实例={}, 返回码={}", scene, inst, ret);
        return false;
    }
    MYLOG_INFO("[NAudioCore] 播放已停止, 场景={}, 实例={}", scene, inst);
    return true;
}

bool NAudioCore::GetPlayInfo(unsigned char scene, unsigned char inst, _play_info& playinfo) {
    int ret = na_get_playinfo(scene, inst, &playinfo);
    return ret == RET_SUCCESS;
}

// ============================================================================
// 回调设置
// ============================================================================

void NAudioCore::SetDeviceStatusCallback(DeviceStatusCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    device_status_cb_ = std::move(cb);
}

void NAudioCore::SetPlayStatusCallback(PlayStatusCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    play_status_cb_ = std::move(cb);
}

void NAudioCore::SetHeartbeatTimeout(unsigned int timeout_sec) {
    na_set_heartbeat_timeout(timeout_sec);
    MYLOG_DEBUG("[NAudioCore] 心跳超时已设置为 {} 秒", timeout_sec);
}

// ============================================================================
// 静态回调分发
// ============================================================================

void NAudioCore::OnDeviceStatusChange(unsigned int devno, unsigned short status) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ && instance_->device_status_cb_) {
        instance_->device_status_cb_(devno, status);
    }
}

void NAudioCore::OnPlayStatusChange(unsigned char scene, unsigned char inst, unsigned int status) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ && instance_->play_status_cb_) {
        instance_->play_status_cb_(scene, inst, status);
    }
}

} // namespace my_audio
