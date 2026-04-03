#include "AudioController.h"

#include "MyAudios.h"
#include "MyLog.h"

namespace {

/**
 * @brief 去除字符串首尾空白
 */
std::string trimCopy(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

/**
 * @brief 从 DTO 字段中提取并校验 device_id
 * @return true 校验通过，device_id 已赋值
 */
bool getRequiredDeviceId(const oatpp::String& field,
                         std::string& device_id,
                         std::string& error) {
    if (!field) {
        error = "缺少字符串参数 device_id";
        return false;
    }
    device_id = trimCopy(field->c_str());
    if (device_id.empty()) {
        error = "参数 device_id 不能为空";
        return false;
    }
    return true;
}

} // namespace

namespace my_api::audio_api {

using namespace my_api::base;

AudioController::AudioController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : BaseApiController(objectMapper) {}

std::shared_ptr<AudioController> AudioController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper) {
    return std::make_shared<AudioController>(objectMapper);
}

// ============================================================================
// 查询类接口
// ============================================================================

/**
 * @brief 查看启动参数（初始化配置）
 *
 * 返回音频设备管理器在初始化时接收到的 JSON 配置快照。
 */
MyAPIResponsePtr AudioController::getAudioConfig() {
    MYLOG_INFO("[API-Audio] GET /v1/audio/config 查看启动参数");

    auto& manager = my_audio::MyAudios::GetInstance();
    auto config = manager.GetInitConfig();

    if (config.empty()) {
        MYLOG_WARN("[API-Audio] 管理器尚未初始化，无启动参数");
        return jsonError(503, "音频管理器尚未初始化，无启动参数");
    }

    MYLOG_INFO("[API-Audio] 启动参数获取成功");
    return jsonOk(config, "启动参数获取成功");
}

/**
 * @brief 查看指定扬声器详细信息
 *
 * 根据 device_id 返回设备的配置、当前状态及硬件信息。
 */
MyAPIResponsePtr AudioController::getAudioInfo(
    const oatpp::Object<my_api::dto::AudioDeviceIdDto>& deviceDto) {
    if (!deviceDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string device_id, error;
    if (!getRequiredDeviceId(deviceDto->device_id, device_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API-Audio] POST /v1/audio/info 查看设备信息: device_id={}", device_id);

    auto& manager = my_audio::MyAudios::GetInstance();

    // 检查设备是否存在
    auto status = manager.Status(device_id);
    auto cfg    = manager.Config(device_id);
    if (cfg.device_id.empty()) {
        MYLOG_WARN("[API-Audio] 设备不存在: {}", device_id);
        return jsonError(404, "扬声器设备不存在: " + device_id);
    }

    auto info = manager.Info(device_id);

    nlohmann::json data;
    data["device_id"] = device_id;
    data["status"]    = my_audio::AudioStatusToString(status);
    data["config"]    = cfg.ToJson();
    data["info"]      = info.ToJson();

    MYLOG_INFO("[API-Audio] 设备信息获取成功: device_id={}, 状态={}",
               device_id, my_audio::AudioStatusToString(status));
    return jsonOk(data, "设备信息获取成功");
}

/**
 * @brief 查看全部扬声器状态
 *
 * 返回所有已注册扬声器的综合状态视图。
 */
MyAPIResponsePtr AudioController::getAudioStatus() {
    MYLOG_INFO("[API-Audio] GET /v1/audio/status 查看全部状态");

    auto& manager = my_audio::MyAudios::GetInstance();
    auto status_all = manager.StatusAll();

    if (status_all.empty()) {
        MYLOG_WARN("[API-Audio] 管理器未初始化或无设备");
        return jsonError(503, "音频管理器未初始化或无设备");
    }

    nlohmann::json data;
    data["device_count"] = status_all.size();
    data["devices"]      = status_all;

    MYLOG_INFO("[API-Audio] 全部状态获取成功, 设备数={}", status_all.size());
    return jsonOk(data, "全部扬声器状态获取成功");
}

/**
 * @brief 查看可用扬声器列表
 *
 * 返回当前处于空闲或工作状态的扬声器 ID 列表。
 */
MyAPIResponsePtr AudioController::getAvailableSpeakers() {
    MYLOG_INFO("[API-Audio] GET /v1/audio/available 查看可用扬声器列表");

    auto& manager = my_audio::MyAudios::GetInstance();
    auto speakers = manager.GetAvailableSpeakers();

    nlohmann::json data;
    data["count"]   = speakers.size();
    data["devices"] = speakers;

    MYLOG_INFO("[API-Audio] 可用扬声器数量: {}", speakers.size());
    return jsonOk(data, "可用扬声器列表获取成功");
}

// ============================================================================
// 控制类接口
// ============================================================================

/**
 * @brief 在指定扬声器上播放音频文件
 *
 * 校验 device_id 和 filepath 参数后，向目标设备发送播放命令。
 */
MyAPIResponsePtr AudioController::playAudioFile(
    const oatpp::Object<my_api::dto::AudioPlayRequestDto>& playDto) {
    if (!playDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string device_id, error;
    if (!getRequiredDeviceId(playDto->device_id, device_id, error)) {
        return jsonError(400, error);
    }

    if (!playDto->filepath) {
        return jsonError(400, "缺少字符串参数 filepath");
    }
    std::string filepath = trimCopy(playDto->filepath->c_str());
    if (filepath.empty()) {
        return jsonError(400, "参数 filepath 不能为空");
    }

    MYLOG_INFO("[API-Audio] POST /v1/audio/play 播放请求: device_id={}, filepath={}",
               device_id, filepath);

    auto& manager = my_audio::MyAudios::GetInstance();

    // 检查设备是否存在
    auto cfg = manager.Config(device_id);
    if (cfg.device_id.empty()) {
        MYLOG_WARN("[API-Audio] 播放失败: 设备不存在 {}", device_id);
        return jsonError(404, "扬声器设备不存在: " + device_id);
    }

    bool ok = manager.PlayOnSpeaker(device_id, filepath);
    if (!ok) {
        MYLOG_ERROR("[API-Audio] 播放失败: device_id={}, filepath={}", device_id, filepath);
        return jsonError(400, "播放命令发送失败",
                         {{"device_id", device_id}, {"filepath", filepath}});
    }

    MYLOG_INFO("[API-Audio] 播放命令发送成功: device_id={}", device_id);
    return jsonOk({{"device_id", device_id}, {"filepath", filepath}}, "播放命令发送成功");
}

/**
 * @brief 设置指定扬声器音量
 *
 * 校验 device_id 和 volume（0-100）后，设置目标设备音量。
 */
MyAPIResponsePtr AudioController::setVolume(
    const oatpp::Object<my_api::dto::AudioVolumeRequestDto>& volumeDto) {
    if (!volumeDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string device_id, error;
    if (!getRequiredDeviceId(volumeDto->device_id, device_id, error)) {
        return jsonError(400, error);
    }

    if (!volumeDto->volume) {
        return jsonError(400, "缺少参数 volume");
    }
    int volume = *volumeDto->volume;
    if (volume < 0 || volume > 100) {
        return jsonError(400, "参数 volume 必须在 0-100 范围内",
                         {{"received", volume}});
    }

    MYLOG_INFO("[API-Audio] POST /v1/audio/volume 设置音量: device_id={}, volume={}",
               device_id, volume);

    auto& manager = my_audio::MyAudios::GetInstance();

    // 检查设备是否存在
    auto cfg = manager.Config(device_id);
    if (cfg.device_id.empty()) {
        MYLOG_WARN("[API-Audio] 设置音量失败: 设备不存在 {}", device_id);
        return jsonError(404, "扬声器设备不存在: " + device_id);
    }

    bool ok = manager.SetVolume(device_id, volume);
    if (!ok) {
        MYLOG_ERROR("[API-Audio] 设置音量失败: device_id={}, volume={}", device_id, volume);
        return jsonError(400, "设置音量失败",
                         {{"device_id", device_id}, {"volume", volume}});
    }

    MYLOG_INFO("[API-Audio] 音量设置成功: device_id={}, volume={}", device_id, volume);
    return jsonOk({{"device_id", device_id}, {"volume", volume}}, "音量设置成功");
}

} // namespace my_api::audio_api
