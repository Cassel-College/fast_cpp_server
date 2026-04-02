#include "MyAudios.h"
#include "NAudio.h"
#include "MyLog.h"

/**
 * @file MyAudios.cpp
 * @brief 扬声器框架 —— 全局音频设备管理器实现
 *
 * 根据 JSON 配置动态创建厂商适配实例，管理全部设备的生命周期。
 */

namespace my_audio {

// ============================================================================
// 析构
// ============================================================================

MyAudios::~MyAudios() {
    Stop();
}

// ============================================================================
// 生命周期管理
// ============================================================================

bool MyAudios::Init(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        MYLOG_WARN("[MyAudios] 已初始化，忽略重复调用");
        return true;
    }

    MYLOG_INFO("[MyAudios] ====== 开始初始化音频设备管理器 ======");
    MYLOG_INFO("[MyAudios] 配置内容: {}", config.dump(4));

    // 保存初始化配置快照
    init_config_ = config;

    try {
        // 解析 audio_args 节点
        if (!config.contains("audio_args") || !config["audio_args"].is_object()) {
            MYLOG_ERROR("[MyAudios] 初始化失败: 配置中缺少 'audio_args' 节点");
            return false;
        }

        const auto& audio_args = config["audio_args"];
        int expected_count = config.value("audio_count", 0);
        int success_count = 0;

        MYLOG_INFO("[MyAudios] 预期设备数量={}, 实际配置数量={}",
                   expected_count, audio_args.size());

        // 遍历每个音频设备配置
        for (auto it = audio_args.begin(); it != audio_args.end(); ++it) {
            const std::string& key = it.key();
            const auto& dev_config = it.value();

            MYLOG_INFO("[MyAudios] --- 正在创建设备: {} ---", key);

            // 获取设备类型
            std::string type = dev_config.value("type", "audio_server");

            // 创建厂商设备实例
            auto device = CreateAudioDevice(type);
            if (!device) {
                MYLOG_ERROR("[MyAudios] 创建设备失败: 不支持的类型={}, 设备键={}", type, key);
                continue;
            }

            // 初始化设备
            std::string config_str = dev_config.dump();
            if (!device->Init(config_str)) {
                MYLOG_ERROR("[MyAudios] 设备初始化失败, 设备键={}", key);
                continue;
            }

            // 使用 device_id 作为索引键
            std::string device_id = device->GetDeviceId();
            speakers_[device_id] = device;
            success_count++;

            MYLOG_INFO("[MyAudios] 设备创建成功: ID={}, 名称={}, 类型={}",
                       device_id, device->GetName(), type);
        }

        initialized_ = true;
        MYLOG_INFO("[MyAudios] ====== 音频设备管理器初始化完成 ======");
        MYLOG_INFO("[MyAudios] 总计: 预期={}, 成功={}, 失败={}",
                   expected_count, success_count, 
                   static_cast<int>(audio_args.size()) - success_count);
        return success_count > 0;

    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyAudios] 初始化异常: {}", e.what());
        return false;
    }
}

bool MyAudios::Start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        MYLOG_ERROR("[MyAudios] 尚未初始化，无法启动");
        return false;
    }

    MYLOG_INFO("[MyAudios] ====== 开始启动所有音频设备 ======");

    int success_count = 0;

    for (auto& [id, device] : speakers_) {
        MYLOG_INFO("[MyAudios] 正在启动设备: ID={}, 名称={}", id, device->GetName());

        if (!device->Start()) {
            MYLOG_ERROR("[MyAudios] 设备启动失败: ID={}", id);
            continue;
        }

        success_count++;
        MYLOG_INFO("[MyAudios] 设备启动成功: ID={}, 状态={}",
                   id, AudioStatusToString(device->Status()));
    }

    MYLOG_INFO("[MyAudios] ====== 设备启动完成, 成功={}/{} ======",
               success_count, speakers_.size());
    return success_count > 0;
}

void MyAudios::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    MYLOG_INFO("[MyAudios] ====== 开始停止所有音频设备 ======");

    // 请求所有设备停止（NAudio::Stop 内部会 join 自己的 loop_thread_）
    for (auto& [id, device] : speakers_) {
        MYLOG_INFO("[MyAudios] 正在停止设备: ID={}", id);
        device->Stop();
    }

    MYLOG_INFO("[MyAudios] ====== 所有音频设备已停止 ======");
}

// ============================================================================
// 状态查询
// ============================================================================

std::map<std::string, std::shared_ptr<BaseAudio>> MyAudios::GetAllSpeakers() {
    std::lock_guard<std::mutex> lock(mutex_);
    return speakers_;
}

std::vector<std::string> MyAudios::GetAvailableSpeakers() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> available;
    for (const auto& [id, device] : speakers_) {
        AudioStatus s = device->Status();
        if (s == AudioStatus::Idle || s == AudioStatus::Working) {
            available.push_back(id);
        }
    }

    MYLOG_DEBUG("[MyAudios] 可用设备数量: {}/{}", available.size(), speakers_.size());
    return available;
}

AudioStatus MyAudios::Status(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = speakers_.find(device_id);
    if (it == speakers_.end()) {
        MYLOG_WARN("[MyAudios] 查询状态: 设备 {} 不存在", device_id);
        return AudioStatus::Offline;
    }
    return it->second->Status();
}

AudioConfig MyAudios::Config(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = speakers_.find(device_id);
    if (it == speakers_.end()) {
        MYLOG_WARN("[MyAudios] 查询配置: 设备 {} 不存在", device_id);
        return AudioConfig{};
    }
    return it->second->Config();
}

AudioInfo MyAudios::Info(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = speakers_.find(device_id);
    if (it == speakers_.end()) {
        MYLOG_WARN("[MyAudios] 查询信息: 设备 {} 不存在", device_id);
        return AudioInfo{};
    }
    return it->second->Info();
}

nlohmann::json MyAudios::StatusAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json result = nlohmann::json::object();
    for (const auto& [id, device] : speakers_) {
        result[id] = {
            {"name",    device->GetName()},
            {"status",  AudioStatusToString(device->Status())},
            {"config",  device->Config().ToJson()},
            {"info",    device->Info().ToJson()}
        };
    }
    return result;
}

nlohmann::json MyAudios::GetInitConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return init_config_;
}

// ============================================================================
// 播放控制
// ============================================================================

bool MyAudios::PlayOnSpeaker(const std::string& device_id, const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = speakers_.find(device_id);
    if (it == speakers_.end()) {
        MYLOG_ERROR("[MyAudios] 播放失败: 设备 {} 不存在", device_id);
        return false;
    }

    MYLOG_INFO("[MyAudios] 在设备 {} 上播放: {}", device_id, filepath);
    return it->second->PlayFile(filepath);
}

int MyAudios::PlayOnAllSpeakers(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);

    int count = 0;
    for (auto& [id, device] : speakers_) {
        AudioStatus s = device->Status();
        if (s == AudioStatus::Idle || s == AudioStatus::Working) {
            if (device->PlayFile(filepath)) {
                count++;
            }
        }
    }

    MYLOG_INFO("[MyAudios] 在所有扬声器上播放: {}, 成功={}", filepath, count);
    return count;
}

bool MyAudios::SetVolume(const std::string& device_id, int volume) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = speakers_.find(device_id);
    if (it == speakers_.end()) {
        MYLOG_ERROR("[MyAudios] 设置音量失败: 设备 {} 不存在", device_id);
        return false;
    }

    return it->second->SetVolume(volume);
}

// ============================================================================
// 工厂方法
// ============================================================================

std::shared_ptr<BaseAudio> MyAudios::CreateAudioDevice(const std::string& type) {
    // 根据类型创建对应厂商的设备实例
    // 如需扩展新厂商，在此添加 else if 分支即可
    if (type == "audio_server") {
        MYLOG_INFO("[MyAudios] 创建 NAudio 类型设备实例");
        return std::make_shared<NAudio>();
    }

    MYLOG_ERROR("[MyAudios] 未知的设备类型: {}", type);
    return nullptr;
}

} // namespace my_audio
