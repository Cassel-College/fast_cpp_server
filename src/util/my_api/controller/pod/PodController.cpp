#include "PodController.h"

#include "pod_manager.h"
#include "MyLog.h"
#include <algorithm>
#include <cctype>

namespace {

std::string trimCopy(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool getRequiredPodId(const oatpp::String& pod_id_field,
                      std::string& pod_id,
                      std::string& error) {
    if (!pod_id_field) {
        error = "缺少字符串参数 pod_id";
        return false;
    }

    pod_id = trimCopy(pod_id_field->c_str());
    if (pod_id.empty()) {
        error = "参数 pod_id 不能为空";
        return false;
    }

    return true;
}

bool ensureManagerReady(std::string& error) {
    auto& manager = PodModule::PodManager::GetInstance();
    if (!manager.IsInitialized()) {
        error = "吊舱管理器未初始化";
        return false;
    }
    return true;
}

std::shared_ptr<PodModule::IPod> getExistingPod(const std::string& pod_id, std::string& error) {
    auto& manager = PodModule::PodManager::GetInstance();
    auto pod = manager.getPod(pod_id);
    if (!pod) {
        error = "吊舱不存在: " + pod_id;
    }
    return pod;
}

nlohmann::json buildPodErrorDetails(PodModule::PodErrorCode code, const std::string& pod_id) {
    nlohmann::json details;
    details["pod_id"] = pod_id;
    details["pod_error_code"] = static_cast<int>(code);
    details["pod_error_name"] = PodModule::podErrorCodeToString(code);
    return details;
}

int mapPodErrorToHttpStatus(PodModule::PodErrorCode code) {
    switch (code) {
        case PodModule::PodErrorCode::POD_NOT_FOUND:
            return 404;
        case PodModule::PodErrorCode::NOT_CONNECTED:
        case PodModule::PodErrorCode::CONNECTION_FAILED:
        case PodModule::PodErrorCode::CONNECTION_TIMEOUT:
            return 503;
        case PodModule::PodErrorCode::PTZ_CONTROL_FAILED:
        case PodModule::PodErrorCode::CAPABILITY_NOT_SUPPORTED:
            return 400;
        default:
            return 400;
    }
}

bool parsePtzAction(const oatpp::String& action_field,
                    int32_t step_value,
                    char& action,
                    PodModule::PTZCommand& cmd,
                    bool& go_home,
                    std::string& error) {
    if (!action_field) {
        error = "缺少字符串参数 action";
        return false;
    }

    std::string action_text = trimCopy(action_field->c_str());
    std::transform(action_text.begin(), action_text.end(), action_text.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (action_text.size() != 1) {
        error = "参数 action 必须是单字符: w/s/a/d/h";
        return false;
    }

    if (step_value <= 0) {
        error = "参数 step 必须大于 0";
        return false;
    }

    action = action_text.front();
    go_home = false;
    cmd = PodModule::PTZCommand{};

    switch (action) {
        case 'w':
            cmd.pitch_speed = static_cast<double>(step_value);
            return true;
        case 's':
            cmd.pitch_speed = -static_cast<double>(step_value);
            return true;
        case 'a':
            cmd.yaw_speed = -static_cast<double>(step_value);
            return true;
        case 'd':
            cmd.yaw_speed = static_cast<double>(step_value);
            return true;
        case 'h':
            go_home = true;
            return true;
        default:
            error = "不支持的 action，必须是 w/s/a/d/h";
            return false;
    }
}

} // namespace

namespace my_api::pod_api {

using namespace my_api::base;

PodController::PodController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : BaseApiController(objectMapper) {}

std::shared_ptr<PodController> PodController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper) {
    return std::make_shared<PodController>(objectMapper);
}

MyAPIResponsePtr PodController::getPodOnline() {
    MYLOG_INFO("[API] Pod GET online");
    auto& manager = PodModule::PodManager::GetInstance();
    nlohmann::json data;
    data["status"] = manager.IsInitialized() ? "alive" : "not_initialized";
    data["pod_count"] = manager.size();
    return jsonOk(data, "pod manager online");
}

MyAPIResponsePtr PodController::getPodStatus() {
    MYLOG_INFO("[API] Pod GET status");
    auto& manager = PodModule::PodManager::GetInstance();
    if (!manager.IsInitialized()) {
        return jsonError(503, "吊舱管理器未初始化");
    }
    return jsonOk(manager.GetStatusSnapshot(), "pod status retrieved");
}

MyAPIResponsePtr PodController::getPodDetail(const oatpp::Object<my_api::dto::PodIdDto>& podDto) {
    std::string error;
    if (!podDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string pod_id;
    if (!getRequiredPodId(podDto->pod_id, pod_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST detail: {}", pod_id);
    auto& manager = PodModule::PodManager::GetInstance();
    auto pod = manager.getPod(pod_id);
    if (!pod) {
        return jsonError(404, "吊舱不存在: " + pod_id);
    }

    auto info = pod->getPodInfo();
    nlohmann::json data;
    data["pod_id"]    = info.pod_id;
    data["pod_name"]  = info.pod_name;
    data["vendor"]    = PodModule::podVendorToString(info.vendor);
    data["ip"]        = info.ip_address;
    data["port"]      = info.port;
    data["state"]     = PodModule::podStateToString(pod->getState());
    data["connected"] = pod->isConnected();

    if (pod->isMonitorRunning()) {
        auto rt = pod->getRuntimeStatus();
        data["is_online"]      = rt.is_online;
        data["last_update_ms"] = rt.last_update_ms;
    }

    return jsonOk(data, "pod detail retrieved");
}

MyAPIResponsePtr PodController::getPodConfig() {
    MYLOG_INFO("[API] Pod GET config");
    auto& manager = PodModule::PodManager::GetInstance();
    return jsonOk(manager.GetInitConfig(), "pod config retrieved");
}

MyAPIResponsePtr PodController::getPodPtzPose(
    const oatpp::Object<my_api::dto::PodPtzPoseQueryDto>& poseQueryDto) {
    if (!poseQueryDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string error;
    std::string pod_id;
    if (!getRequiredPodId(poseQueryDto->pod_id, pod_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST ptz pose: {}", pod_id);

    if (!ensureManagerReady(error)) {
        return jsonError(503, error);
    }

    auto pod = getExistingPod(pod_id, error);
    if (!pod) {
        return jsonError(404, error);
    }

    auto result = pod->getPose();
    if (!result.isSuccess() || !result.data.has_value()) {
        return jsonError(
            mapPodErrorToHttpStatus(result.code),
            result.message.empty() ? "获取吊舱云台姿态失败" : result.message,
            buildPodErrorDetails(result.code, pod_id));
    }

    const auto& pose = result.data.value();
    nlohmann::json data;
    data["pod_id"] = pod_id;
    data["connected"] = pod->isConnected();
    data["state"] = PodModule::podStateToString(pod->getState());
    data["yaw"] = pose.yaw;
    data["pitch"] = pose.pitch;
    data["roll"] = pose.roll;
    data["zoom"] = pose.zoom;
    return jsonOk(data, "吊舱云台姿态获取成功");
}

MyAPIResponsePtr PodController::connectPod(const oatpp::Object<my_api::dto::PodIdDto>& podDto) {
    std::string error;
    if (!podDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string pod_id;
    if (!getRequiredPodId(podDto->pod_id, pod_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST connect: {}", pod_id);
    auto& manager = PodModule::PodManager::GetInstance();
    auto result = manager.connectPod(pod_id);
    if (result.isSuccess()) {
        return jsonOk({{"pod_id", pod_id}}, "吊舱连接成功");
    }
    return jsonError(400, result.message);
}

MyAPIResponsePtr PodController::disconnectPod(const oatpp::Object<my_api::dto::PodIdDto>& podDto) {
    std::string error;
    if (!podDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string pod_id;
    if (!getRequiredPodId(podDto->pod_id, pod_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST disconnect: {}", pod_id);
    auto& manager = PodModule::PodManager::GetInstance();
    auto result = manager.disconnectPod(pod_id);
    if (result.isSuccess()) {
        return jsonOk({{"pod_id", pod_id}}, "吊舱断开成功");
    }
    return jsonError(400, result.message);
}

MyAPIResponsePtr PodController::controlPodPtz(
    const oatpp::Object<my_api::dto::PodPtzControlDto>& controlDto) {
    if (!controlDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string error;
    std::string pod_id;
    if (!getRequiredPodId(controlDto->pod_id, pod_id, error)) {
        return jsonError(400, error);
    }

    if (!ensureManagerReady(error)) {
        return jsonError(503, error);
    }

    auto pod = getExistingPod(pod_id, error);
    if (!pod) {
        return jsonError(404, error);
    }

    const int32_t step = controlDto->step ? controlDto->step.getValue(10) : 10;
    char action = '\0';
    bool go_home = false;
    PodModule::PTZCommand command;
    if (!parsePtzAction(controlDto->action, step, action, command, go_home, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST ptz control: pod_id={}, action={}, step={}", pod_id, action, step);

    auto result = go_home ? pod->goHome() : pod->controlSpeed(command);
    if (!result.isSuccess()) {
        return jsonError(
            mapPodErrorToHttpStatus(result.code),
            result.message.empty() ? "吊舱 PTZ 控制失败" : result.message,
            buildPodErrorDetails(result.code, pod_id));
    }

    nlohmann::json data;
    data["pod_id"] = pod_id;
    data["action"] = std::string(1, action);
    data["step"] = step;
    data["mode"] = go_home ? "home" : "speed";
    if (!go_home) {
        data["command"] = {
            {"yaw_speed", command.yaw_speed},
            {"pitch_speed", command.pitch_speed},
            {"zoom_speed", command.zoom_speed}
        };
    }
    data["connected"] = pod->isConnected();
    data["state"] = PodModule::podStateToString(pod->getState());
    return jsonOk(data, go_home ? "吊舱回中指令下发成功" : "吊舱 PTZ 控制指令下发成功");
}

MyAPIResponsePtr PodController::listPodIds() {
    MYLOG_INFO("[API] Pod GET list");
    auto& manager = PodModule::PodManager::GetInstance();
    auto ids = manager.listPodIds();
    nlohmann::json data;
    data["pod_ids"] = ids;
    data["count"]   = ids.size();
    return jsonOk(data, "pod list retrieved");
}

} // namespace my_api::pod_api
