#include "PodController.h"

#include "pod_manager.h"
#include "MyLog.h"

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

MyAPIResponsePtr PodController::getPodPtzPose(const oatpp::String& pod_id_path) {
    std::string error;
    std::string pod_id;
    if (!getRequiredPodId(pod_id_path, pod_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod GET ptz pose: {}", pod_id);

    auto& manager = PodModule::PodManager::GetInstance();
    if (!manager.IsInitialized()) {
        return jsonError(503, "吊舱管理器未初始化");
    }

    auto pod = manager.getPod(pod_id);
    if (!pod) {
        return jsonError(404, "吊舱不存在: " + pod_id);
    }

    auto result = pod->getPose();
    if (!result.isSuccess() || !result.data.has_value()) {
        return jsonError(400, result.message.empty() ? "获取吊舱云台姿态失败" : result.message);
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

MyAPIResponsePtr PodController::setPodPtzPose(
    const oatpp::String& pod_id_path,
    const oatpp::Object<my_api::dto::PodPtzPoseDto>& poseDto) {
    if (!poseDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string error;
    std::string pod_id;
    if (!getRequiredPodId(pod_id_path, pod_id, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST ptz pose: {}", pod_id);

    auto& manager = PodModule::PodManager::GetInstance();
    if (!manager.IsInitialized()) {
        return jsonError(503, "吊舱管理器未初始化");
    }

    auto pod = manager.getPod(pod_id);
    if (!pod) {
        return jsonError(404, "吊舱不存在: " + pod_id);
    }

    PodModule::PTZPose target_pose;
    target_pose.yaw = poseDto->yaw ? poseDto->yaw.getValue(0.0) : 0.0;
    target_pose.pitch = poseDto->pitch ? poseDto->pitch.getValue(0.0) : 0.0;
    target_pose.roll = poseDto->roll ? poseDto->roll.getValue(0.0) : 0.0;
    target_pose.zoom = poseDto->zoom ? poseDto->zoom.getValue(1.0) : 1.0;

    auto result = pod->setPose(target_pose);
    if (!result.isSuccess()) {
        return jsonError(400, result.message.empty() ? "控制吊舱云台姿态失败" : result.message);
    }

    nlohmann::json data;
    data["pod_id"] = pod_id;
    data["target_pose"] = {
        {"yaw", target_pose.yaw},
        {"pitch", target_pose.pitch},
        {"roll", target_pose.roll},
        {"zoom", target_pose.zoom}
    };
    data["connected"] = pod->isConnected();
    data["state"] = PodModule::podStateToString(pod->getState());
    return jsonOk(data, "吊舱云台姿态控制指令下发成功");
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
