#pragma once

/**
 * @file pod_registry.h
 * @brief 吊舱注册表
 * 
 * 管理所有已注册的吊舱设备实例。
 */

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace PodModule {

// 前向声明
class IPod;

/**
 * @brief 吊舱注册表
 * 
 * 负责管理所有吊舱设备实例：
 * - 注册 Pod
 * - 删除 Pod
 * - 查询 Pod
 * - 列出所有 Pod
 */
class PodRegistry {
public:
    PodRegistry();
    ~PodRegistry();

    /**
     * @brief 注册一个 Pod
     * @param pod_id 吊舱唯一标识
     * @param pod 吊舱实例
     * @return 是否注册成功
     */
    bool registerPod(const std::string& pod_id, std::shared_ptr<IPod> pod);

    /**
     * @brief 注销一个 Pod
     * @param pod_id 吊舱唯一标识
     * @return 是否注销成功
     */
    bool unregisterPod(const std::string& pod_id);

    /**
     * @brief 根据 ID 查询 Pod
     * @param pod_id 吊舱唯一标识
     * @return Pod 实例，不存在返回 nullptr
     */
    std::shared_ptr<IPod> getPod(const std::string& pod_id) const;

    /**
     * @brief 判断 Pod 是否存在
     */
    bool hasPod(const std::string& pod_id) const;

    /**
     * @brief 获取所有 Pod ID 列表
     */
    std::vector<std::string> listPodIds() const;

    /**
     * @brief 获取所有 Pod 实例列表
     */
    std::vector<std::shared_ptr<IPod>> listPods() const;

    /**
     * @brief 获取注册数量
     */
    size_t size() const;

    /**
     * @brief 清空所有 Pod
     */
    void clear();

private:
    std::unordered_map<std::string, std::shared_ptr<IPod>> pods_;
};

} // namespace PodModule
