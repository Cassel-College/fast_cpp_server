/**
 * @file pod_manager.cpp
 * @brief 吊舱管理器实现
 */

#include "pod_manager.h"
#include "MyLog.h"

namespace PodModule {

PodManager::PodManager() {
    MYLOG_INFO("[吊舱管理器] PodManager 构造");
}

PodManager::~PodManager() {
    MYLOG_INFO("[吊舱管理器] PodManager 析构");
}

PodResult<void> PodManager::addPod(std::shared_ptr<IPod> pod) {
    if (!pod) {
        MYLOG_ERROR("[吊舱管理器] 添加失败：吊舱实例为空");
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, "吊舱实例为空");
    }

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
    if (!registry_.hasPod(pod_id)) {
        MYLOG_WARN("[吊舱管理器] 移除失败：吊舱不存在: {}", pod_id);
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND);
    }

    // 先断开连接
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
    return registry_.getPod(pod_id);
}

std::vector<std::shared_ptr<IPod>> PodManager::listPods() const {
    return registry_.listPods();
}

std::vector<std::string> PodManager::listPodIds() const {
    return registry_.listPodIds();
}

size_t PodManager::size() const {
    return registry_.size();
}

} // namespace PodModule
