/**
 * @file pod_registry.cpp
 * @brief 吊舱注册表实现
 */

#include "pod_registry.h"
#include "MyLog.h"

namespace PodModule {

PodRegistry::PodRegistry() {
    MYLOG_DEBUG("[吊舱注册表] PodRegistry 构造");
}

PodRegistry::~PodRegistry() {
    MYLOG_DEBUG("[吊舱注册表] PodRegistry 析构，清理 {} 个吊舱", pods_.size());
    pods_.clear();
}

bool PodRegistry::registerPod(const std::string& pod_id, std::shared_ptr<IPod> pod) {
    if (!pod) {
        MYLOG_ERROR("[吊舱注册表] 注册失败：吊舱实例为空，ID: {}", pod_id);
        return false;
    }

    auto it = pods_.find(pod_id);
    if (it != pods_.end()) {
        MYLOG_WARN("[吊舱注册表] 吊舱已存在，将覆盖: {}", pod_id);
    }

    pods_[pod_id] = pod;
    MYLOG_INFO("[吊舱注册表] 注册吊舱成功: {}", pod_id);
    return true;
}

bool PodRegistry::unregisterPod(const std::string& pod_id) {
    auto it = pods_.find(pod_id);
    if (it == pods_.end()) {
        MYLOG_WARN("[吊舱注册表] 注销失败：吊舱不存在: {}", pod_id);
        return false;
    }
    pods_.erase(it);
    MYLOG_INFO("[吊舱注册表] 注销吊舱成功: {}", pod_id);
    return true;
}

std::shared_ptr<IPod> PodRegistry::getPod(const std::string& pod_id) const {
    auto it = pods_.find(pod_id);
    if (it == pods_.end()) {
        return nullptr;
    }
    return it->second;
}

bool PodRegistry::hasPod(const std::string& pod_id) const {
    return pods_.find(pod_id) != pods_.end();
}

std::vector<std::string> PodRegistry::listPodIds() const {
    std::vector<std::string> ids;
    ids.reserve(pods_.size());
    for (const auto& pair : pods_) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::shared_ptr<IPod>> PodRegistry::listPods() const {
    std::vector<std::shared_ptr<IPod>> result;
    result.reserve(pods_.size());
    for (const auto& pair : pods_) {
        result.push_back(pair.second);
    }
    return result;
}

size_t PodRegistry::size() const {
    return pods_.size();
}

void PodRegistry::clear() {
    MYLOG_INFO("[吊舱注册表] 清空所有吊舱，共 {} 个", pods_.size());
    pods_.clear();
}

} // namespace PodModule
