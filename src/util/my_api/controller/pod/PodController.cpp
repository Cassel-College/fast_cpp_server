#include "PodController.h"

#include "pod_manager.h"
#include "MyLog.h"

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

MyAPIResponsePtr PodController::getPodDetail(const oatpp::String& body) {
    auto reqJson = nlohmann::json::parse(body->c_str());
    std::string pod_id = reqJson.value("pod_id", "");
    MYLOG_INFO("[API] Pod POST detail: {}", pod_id);
    if (pod_id.empty()) {
        return jsonError(400, "缺少参数 pod_id");
    }
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

    nlohmann::json caps = nlohmann::json::array();
    for (auto ct : pod->listCapabilities()) {
        caps.push_back(PodModule::capabilityTypeToString(ct));
    }
    data["capabilities"] = caps;

    return jsonOk(data, "pod detail retrieved");
}

MyAPIResponsePtr PodController::getPodConfig() {
    MYLOG_INFO("[API] Pod GET config");
    auto& manager = PodModule::PodManager::GetInstance();
    return jsonOk(manager.GetInitConfig(), "pod config retrieved");
}

MyAPIResponsePtr PodController::connectPod(const oatpp::String& body) {
    auto reqJson = nlohmann::json::parse(body->c_str());
    std::string pod_id = reqJson.value("pod_id", "");
    MYLOG_INFO("[API] Pod POST connect: {}", pod_id);
    if (pod_id.empty()) {
        return jsonError(400, "缺少参数 pod_id");
    }
    auto& manager = PodModule::PodManager::GetInstance();
    auto result = manager.connectPod(pod_id);
    if (result.isSuccess()) {
        return jsonOk({{"pod_id", pod_id}}, "吊舱连接成功");
    }
    return jsonError(400, result.message);
}

MyAPIResponsePtr PodController::disconnectPod(const oatpp::String& body) {
    auto reqJson = nlohmann::json::parse(body->c_str());
    std::string pod_id = reqJson.value("pod_id", "");
    MYLOG_INFO("[API] Pod POST disconnect: {}", pod_id);
    if (pod_id.empty()) {
        return jsonError(400, "缺少参数 pod_id");
    }
    auto& manager = PodModule::PodManager::GetInstance();
    auto result = manager.disconnectPod(pod_id);
    if (result.isSuccess()) {
        return jsonOk({{"pod_id", pod_id}}, "吊舱断开成功");
    }
    return jsonError(400, result.message);
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
