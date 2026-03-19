/**
 * @file pod_manager.cpp
 * @brief 吊舱管理器实现
 */

#include "pod_manager.h"
#include "../pod/dji/dji_pod.h"
#include "../pod/pinling/pinling_pod.h"
#include "MyLog.h"

namespace PodModule {

PodManager::PodManager() {
    MYLOG_INFO("[吊舱管理器] PodManager 构造");
}

PodManager::~PodManager() {
    MYLOG_INFO("[吊舱管理器] PodManager 析构");
}

bool PodManager::Init(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_.load()) {
        MYLOG_WARN("[吊舱管理器] 已经初始化过，跳过重复初始化");
        return true;
    }

    init_config_ = config;
    MYLOG_INFO("[吊舱管理器] 开始初始化，配置: {}", config.dump(4));

    // 解析 pod_args
    if (!config.contains("pod_args") || !config["pod_args"].is_object()) {
        MYLOG_WARN("[吊舱管理器] 配置中无 pod_args，初始化为空管理器");
        initialized_.store(true);
        return true;
    }

    const auto& pod_args = config["pod_args"];
    for (auto it = pod_args.begin(); it != pod_args.end(); ++it) {
        const std::string& pod_id = it.key();
        const auto& pod_cfg = it.value();

        auto pod = createPod(pod_id, pod_cfg);
        if (pod) {
            pod->initializeCapabilities();
            registry_.registerPod(pod_id, pod);
            MYLOG_INFO("[吊舱管理器] 初始化吊舱: {} ({})",
                       pod_id, podVendorToString(pod->getVendor()));
        } else {
            MYLOG_ERROR("[吊舱管理器] 创建吊舱失败: {}", pod_id);
        }
    }

    initialized_.store(true);
    MYLOG_INFO("[吊舱管理器] 初始化完成，共 {} 个吊舱", registry_.size());
    return true;
}

nlohmann::json PodManager::GetInitConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return init_config_;
}

std::shared_ptr<IPod> PodManager::createPod(const std::string& pod_id, const nlohmann::json& pod_cfg) {
    std::string type = pod_cfg.value("type", "");
    std::string name = pod_cfg.value("name", pod_id);
    std::string ip   = pod_cfg.value("ip", "");
    int port         = pod_cfg.value("port", 0);

    PodInfo info;
    info.pod_id     = pod_id;
    info.pod_name   = name;
    info.ip_address = ip;
    info.port       = port;

    if (type == "dji") {
        info.vendor = PodVendor::DJI;
        return std::make_shared<DjiPod>(info);
    } else if (type == "pinling") {
        info.vendor = PodVendor::PINLING;
        return std::make_shared<PinlingPod>(info);
    } else {
        MYLOG_ERROR("[吊舱管理器] 未知吊舱类型: {}，pod_id={}", type, pod_id);
        return nullptr;
    }
}

PodResult<void> PodManager::addPod(std::shared_ptr<IPod> pod) {
    if (!pod) {
        MYLOG_ERROR("[吊舱管理器] 添加失败：吊舱实例为空");
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, "吊舱实例为空");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string pod_id = pod->getPodId();
    if (registry_.hasPod(pod_id)) {
        MYLOG_WARN("[吊舱管理器] 吊舱已存在: {}", pod_id);
        return PodResult<void>::fail(PodErrorCode::POD_ALREADY_EXISTS);
    }

    registry_.registerPod(pod_id, pod);
    MYLOG_INFO("[吊舱管理器] 添加吊舱成功: {} ({})",
               pod_id, podVendorToString(pod->getVendor()));
    return PodResult<void>::success("添加吊舱成功");
}

PodResult<void> PodManager::removePod(const std::string& pod_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!registry_.hasPod(pod_id)) {
        MYLOG_WARN("[吊舱管理器] 移除失败：吊舱不存在: {}", pod_id);
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND);
    }

    auto pod = registry_.getPod(pod_id);
    if (pod && pod->isConnected()) {
        MYLOG_INFO("[吊舱管理器] 移除前断开吊舱连接: {}", pod_id);
        pod->disconnect();
    }

    registry_.unregisterPod(pod_id);
    MYLOG_INFO("[吊舱管理器] 移除吊舱成功: {}", pod_id);
    return PodResult<void>::success("移除吊舱成功");
}

std::shared_ptr<IPod> PodManager::getPod(const std::string& pod_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.getPod(pod_id);
}

std::vector<std::shared_ptr<IPod>> PodManager::listPods() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.listPods();
}

std::vector<std::string> PodManager::listPodIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.listPodIds();
}

size_t PodManager::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.size();
}

PodResult<void> PodManager::connectPod(const std::string& pod_id) {
    auto pod = getPod(pod_id);
    if (!pod) {
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND, "吊舱不存在: " + pod_id);
    }
    return pod->connect();
}

PodResult<void> PodManager::disconnectPod(const std::string& pod_id) {
    auto pod = getPod(pod_id);
    if (!pod) {
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND, "吊舱不存在: " + pod_id);
    }
    return pod->disconnect();
}

nlohmann::json PodManager::GetStatusSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json result = nlohmann::json::array();

    auto pods = registry_.listPods();
    for (const auto& pod : pods) {
        nlohmann::json item;
        auto info = pod->getPodInfo();
        item["pod_id"]    = info.pod_id;
        item["pod_name"]  = info.pod_name;
        item["vendor"]    = podVendorToString(info.vendor);
        item["ip"]        = info.ip_address;
        item["port"]      = info.port;
        item["state"]     = podStateToString(pod->getState());
        item["connected"] = pod->isConnected();

        // 列出已注册能力
        nlohmann::json caps = nlohmann::json::array();
        for (auto ct : pod->listCapabilities()) {
            caps.push_back(capabilityTypeToString(ct));
        }
        item["capabilities"] = caps;
        result.push_back(item);
    }
    return result;
}

} // namespace PodModule
