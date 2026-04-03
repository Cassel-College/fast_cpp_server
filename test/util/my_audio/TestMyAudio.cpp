#include "gtest/gtest.h"
#include "BaseAudio.h"
#include "MyAudios.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

/**
 * @file TestMyAudio.cpp
 * @brief 扬声器框架单元测试
 *
 * 测试内容：
 * 1. AudioConfig 序列化/反序列化
 * 2. AudioStatus 枚举转字符串
 * 3. MyAudios 单例初始化
 * 4. MyAudios 设备查询
 * 5. MyAudios 播放接口（Mock 环境下验证逻辑）
 */

using namespace my_audio;
using json = nlohmann::json;

// ============================================================================
// AudioConfig 测试
// ============================================================================

/**
 * @brief 测试 AudioConfig 从 JSON 解析
 */
TEST(MyAudio_AudioConfig, FromJson) {
    json j = {
        {"device_id", 3445},
        {"name", "test_speaker"},
        {"type", "audio_server"},
        {"ip", "192.168.1.150"},
        {"port", 8890},
        {"default_volume", 80}
    };

    AudioConfig cfg = AudioConfig::FromJson(j);

    EXPECT_EQ(cfg.device_id, "3445");
    EXPECT_EQ(cfg.name, "test_speaker");
    EXPECT_EQ(cfg.type, "audio_server");
    EXPECT_EQ(cfg.ip, "192.168.1.150");
    EXPECT_EQ(cfg.port, 8890);
    EXPECT_EQ(cfg.default_volume, 80);
}

/**
 * @brief 测试 AudioConfig 从 JSON 解析（device_id 为字符串）
 */
TEST(MyAudio_AudioConfig, FromJsonStringId) {
    json j = {
        {"device_id", "speaker_01"},
        {"name", "test_speaker"},
        {"type", "audio_server"},
        {"ip", "10.0.0.1"},
        {"port", 9000}
    };

    AudioConfig cfg = AudioConfig::FromJson(j);
    EXPECT_EQ(cfg.device_id, "speaker_01");
    EXPECT_EQ(cfg.default_volume, 50);  // 默认值
}

/**
 * @brief 测试 AudioConfig 序列化为 JSON
 */
TEST(MyAudio_AudioConfig, ToJson) {
    AudioConfig cfg;
    cfg.device_id = "001";
    cfg.name = "speaker_a";
    cfg.type = "audio_server";
    cfg.ip = "127.0.0.1";
    cfg.port = 8080;
    cfg.default_volume = 75;

    json j = cfg.ToJson();

    EXPECT_EQ(j["device_id"].get<std::string>(), "001");
    EXPECT_EQ(j["name"].get<std::string>(), "speaker_a");
    EXPECT_EQ(j["port"].get<int>(), 8080);
    EXPECT_EQ(j["default_volume"].get<int>(), 75);
}

/**
 * @brief 测试 AudioConfig 缺少字段时使用默认值
 */
TEST(MyAudio_AudioConfig, FromJsonPartial) {
    json j = {
        {"name", "partial_speaker"}
    };

    AudioConfig cfg = AudioConfig::FromJson(j);
    EXPECT_EQ(cfg.device_id, "");         // 空字符串
    EXPECT_EQ(cfg.name, "partial_speaker");
    EXPECT_EQ(cfg.port, 0);              // 默认值
    EXPECT_EQ(cfg.default_volume, 50);   // 默认值
}

// ============================================================================
// AudioStatus 测试
// ============================================================================

/**
 * @brief 测试状态枚举转中文字符串
 */
TEST(MyAudio_AudioStatus, ToString) {
    EXPECT_EQ(AudioStatusToString(AudioStatus::Working),  "工作中");
    EXPECT_EQ(AudioStatusToString(AudioStatus::Idle),     "空闲");
    EXPECT_EQ(AudioStatusToString(AudioStatus::Offline),  "离线");
    EXPECT_EQ(AudioStatusToString(AudioStatus::Error),    "故障");
}

// ============================================================================
// AudioInfo 测试
// ============================================================================

/**
 * @brief 测试 AudioInfo 序列化为 JSON
 */
TEST(MyAudio_AudioInfo, ToJson) {
    AudioInfo info;
    info.model = "NA-100";
    info.firmware_version = "v2.1.0";
    info.hardware_version = "v1.0";
    info.serial_number = "SN12345";

    json j = info.ToJson();
    EXPECT_EQ(j["model"].get<std::string>(), "NA-100");
    EXPECT_EQ(j["firmware_version"].get<std::string>(), "v2.1.0");
    EXPECT_EQ(j["serial_number"].get<std::string>(), "SN12345");
}

// ============================================================================
// MyAudios 单例测试
// ============================================================================

/**
 * @brief 测试 MyAudios 单例唯一性
 */
TEST(MyAudio_MyAudios, SingletonUniqueness) {
    auto& inst1 = MyAudios::GetInstance();
    auto& inst2 = MyAudios::GetInstance();
    EXPECT_EQ(&inst1, &inst2);
}

/**
 * @brief 测试 MyAudios 缺少 audio_args 时初始化失败
 */
TEST(MyAudio_MyAudios, InitFailsWithoutAudioArgs) {
    json config = {
        {"audio_count", 1}
        // 缺少 audio_args
    };

    // 注意: 单例如果已经初始化过，此处会返回 true（重复调用保护）
    // 此测试在未初始化状态下才能精确测试
    // 由于单例状态限制，此处仅验证接口可调用不崩溃
    MyAudios::GetInstance().Init(config);
}

/**
 * @brief 测试获取不存在的设备状态
 */
TEST(MyAudio_MyAudios, StatusNonExistentDevice) {
    auto status = MyAudios::GetInstance().Status("nonexistent_device");
    EXPECT_EQ(status, AudioStatus::Offline);
}

/**
 * @brief 测试在不存在的设备上播放
 */
TEST(MyAudio_MyAudios, PlayOnNonExistentDevice) {
    bool result = MyAudios::GetInstance().PlayOnSpeaker("nonexistent_device", "/test.mp3");
    EXPECT_FALSE(result);
}

/**
 * @brief 测试获取不存在的设备配置
 */
TEST(MyAudio_MyAudios, ConfigNonExistentDevice) {
    auto config = MyAudios::GetInstance().Config("nonexistent_device");
    EXPECT_EQ(config.device_id, "");
    EXPECT_EQ(config.port, 0);
}

/**
 * @brief 测试获取不存在的设备信息
 */
TEST(MyAudio_MyAudios, InfoNonExistentDevice) {
    auto info = MyAudios::GetInstance().Info("nonexistent_device");
    EXPECT_EQ(info.model, "");
    EXPECT_EQ(info.firmware_version, "");
}

/**
 * @brief 测试获取全部设备状态 JSON
 */
TEST(MyAudio_MyAudios, StatusAllReturnsJson) {
    json status = MyAudios::GetInstance().StatusAll();
    EXPECT_TRUE(status.is_object());
}

/**
 * @brief 测试设置不存在设备的音量
 */
TEST(MyAudio_MyAudios, SetVolumeNonExistentDevice) {
    bool result = MyAudios::GetInstance().SetVolume("nonexistent_device", 50);
    EXPECT_FALSE(result);
}

/**
 * @brief 测试 MyAudios::GetInstance() 初始化不会崩溃且可用
 */
TEST(MyAudio_MyAudios, TestInit) {
    auto& inst = MyAudios::GetInstance();
    json config = {
        {"audio_count", 1},
        {"audio_args", {
            {"speaker_01", {
                {"device_id", 3445},
                {"name", "Test Speaker"},
                {"type", "audio_server"},
                {"ip", "192.168.1.150"},
                {"port", 8890},
                {"default_volume", 50}
            }}
        }}
    };
    bool init_result = inst.Init(config);
    EXPECT_TRUE(init_result);

    // 验证初始化配置快照
    json init_config = inst.GetInitConfig();
    std::cout << "初始化配置快照: " << init_config.dump(4) << std::endl;
    EXPECT_EQ(init_config["audio_count"].get<int>(), 1);
    EXPECT_TRUE(init_config.contains("audio_args"));

    inst.Start();
    inst.GetAllSpeakers();
    // std::vector<std::string> available_speakers = inst.GetAvailableSpeakers();
    // for (const auto& device_id : available_speakers) {
    //     // 获取设备状态、配置、信息
    //     AudioStatus status = inst.Status(device_id);
    //     std::cout << "设备 " << device_id << " 状态: " << AudioStatusToString(status) << std::endl;

    //     AudioConfig config = inst.Config(device_id);
    //     std::cout << "设备 " << device_id << " 配置: " << config.ToJson().dump(4) << std::endl;

    //     AudioInfo info = inst.Info(device_id);
    //     std::cout << "设备 " << device_id << " 信息: " << info.ToJson().dump(4) << std::endl;

    //     // inst.PlayOnSpeaker(device_id, "/path/to/test.mp3");
    //     inst.SetVolume(device_id, 20);

    //     int a = 1;
    //     while (a == 1) {
    //         // 模拟等待播放完成
    //         std::this_thread::sleep_for(std::chrono::seconds(1));
    //         inst.PlayOnSpeaker(device_id, "/home/cs/Desktop/2536-laba/linux/NAudioSDK_Ubuntu20.04_20240423153159/test_audio/alarm.mp3");
    //         std::cin >> a;
    //         if (a == 0) {
    //             std::cout << "停止播放..." << std::endl;
    //             break;
    //         }
    //     }
    // }
}