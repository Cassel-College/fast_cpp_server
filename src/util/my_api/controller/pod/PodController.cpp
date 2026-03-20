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

bool getCapabilityTypeField(const oatpp::Object<my_api::dto::PodCapabilityQueryDto>& query_dto,
                            PodModule::CapabilityType& capability_type,
                            std::string& error) {
    if (!query_dto || !query_dto->capability_type) {
        error = "缺少参数 capability_type";
        return false;
    }

    const auto stored_type = query_dto->capability_type.getStoredType();

    if (stored_type == oatpp::String::Class::getType()) {
        const auto capability_text = trimCopy(query_dto->capability_type.retrieve<oatpp::String>().getValue(""));
        if (!PodModule::capabilityTypeFromString(capability_text, capability_type) ||
            capability_type == PodModule::CapabilityType::UNKNOWN) {
            error = "不支持的 capability_type: " + capability_text;
            return false;
        }
        return true;
    }

    if (stored_type == oatpp::Float64::Class::getType()) {
        const auto capability_number = query_dto->capability_type.retrieve<oatpp::Float64>().getValue(-1.0);
        const auto capability_index = static_cast<int>(capability_number);
        if (capability_number != static_cast<double>(capability_index)) {
            error = "参数 capability_type 必须为整数编号";
            return false;
        }

        capability_type = static_cast<PodModule::CapabilityType>(capability_index);
        if (capability_type == PodModule::CapabilityType::UNKNOWN ||
            PodModule::capabilityTypeToKey(capability_type) == "UNKNOWN") {
            error = "不支持的 capability_type";
            return false;
        }
        return true;
    }

    error = "参数 capability_type 必须为字符串或整数";
    return false;
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

    nlohmann::json caps = nlohmann::json::array();
    for (auto ct : pod->listCapabilities()) {
        caps.push_back(PodModule::capabilityTypeToString(ct));
    }
    data["capabilities"] = caps;

    return jsonOk(data, "pod detail retrieved");
}

MyAPIResponsePtr PodController::getPodCapability(const oatpp::Object<my_api::dto::PodCapabilityQueryDto>& queryDto) {
    std::string error;
    if (!queryDto) {
        return jsonError(400, "请求体不能为空");
    }

    std::string pod_id;
    if (!getRequiredPodId(queryDto->pod_id, pod_id, error)) {
        return jsonError(400, error);
    }

    PodModule::CapabilityType capability_type = PodModule::CapabilityType::UNKNOWN;
    if (!getCapabilityTypeField(queryDto, capability_type, error)) {
        return jsonError(400, error);
    }

    MYLOG_INFO("[API] Pod POST capability: pod_id={}, capability={}",
               pod_id, PodModule::capabilityTypeToKey(capability_type));

    auto& manager = PodModule::PodManager::GetInstance();
    auto pod = manager.getPod(pod_id);
    if (!pod) {
        return jsonError(404, "吊舱不存在: " + pod_id);
    }

    auto capability = pod->getCapability(capability_type);
    nlohmann::json data;
    data["pod_id"] = pod_id;
    data["capability_type"] = static_cast<int>(capability_type);
    data["capability_key"] = PodModule::capabilityTypeToKey(capability_type);
    data["capability_name"] = PodModule::capabilityTypeToString(capability_type);
    data["supported"] = capability != nullptr;

    if (capability) {
        data["registered_name"] = capability->getName();
    }

    return jsonOk(data, "pod capability retrieved");
}

MyAPIResponsePtr PodController::getPodConfig() {
    MYLOG_INFO("[API] Pod GET config");
    auto& manager = PodModule::PodManager::GetInstance();
    return jsonOk(manager.GetInitConfig(), "pod config retrieved");
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
