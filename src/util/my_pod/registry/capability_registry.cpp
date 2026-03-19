/**
 * @file capability_registry.cpp
 * @brief 能力注册表实现
 */

#include "capability_registry.h"
#include "MyLog.h"

namespace PodModule {

CapabilityRegistry::CapabilityRegistry() {
    MYLOG_DEBUG("[能力注册表] CapabilityRegistry 构造");
}

CapabilityRegistry::~CapabilityRegistry() {
    MYLOG_DEBUG("[能力注册表] CapabilityRegistry 析构，清理 {} 个能力", capabilities_.size());
    capabilities_.clear();
}

bool CapabilityRegistry::registerCapability(CapabilityType type, std::shared_ptr<ICapability> capability) {
    if (!capability) {
        MYLOG_ERROR("[能力注册表] 注册失败：能力实例为空，类型: {}", capabilityTypeToString(type));
        return false;
    }

    auto it = capabilities_.find(type);
    if (it != capabilities_.end()) {
        MYLOG_WARN("[能力注册表] 能力已存在，将覆盖: {}", capabilityTypeToString(type));
    }

    capabilities_[type] = capability;
    MYLOG_INFO("[能力注册表] 注册能力成功: {}", capabilityTypeToString(type));
    return true;
}

bool CapabilityRegistry::unregisterCapability(CapabilityType type) {
    auto it = capabilities_.find(type);
    if (it == capabilities_.end()) {
        MYLOG_WARN("[能力注册表] 注销失败：能力不存在: {}", capabilityTypeToString(type));
        return false;
    }
    capabilities_.erase(it);
    MYLOG_INFO("[能力注册表] 注销能力成功: {}", capabilityTypeToString(type));
    return true;
}

std::shared_ptr<ICapability> CapabilityRegistry::getCapability(CapabilityType type) const {
    auto it = capabilities_.find(type);
    if (it == capabilities_.end()) {
        return nullptr;
    }
    return it->second;
}

bool CapabilityRegistry::hasCapability(CapabilityType type) const {
    return capabilities_.find(type) != capabilities_.end();
}

std::vector<CapabilityType> CapabilityRegistry::listCapabilities() const {
    std::vector<CapabilityType> types;
    types.reserve(capabilities_.size());
    for (const auto& pair : capabilities_) {
        types.push_back(pair.first);
    }
    return types;
}

size_t CapabilityRegistry::size() const {
    return capabilities_.size();
}

void CapabilityRegistry::clear() {
    MYLOG_INFO("[能力注册表] 清空所有能力，共 {} 个", capabilities_.size());
    capabilities_.clear();
}

} // namespace PodModule
