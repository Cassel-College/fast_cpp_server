#pragma once

/**
 * @file pod_manager.h
 * @brief 吊舱管理器
 * 
 * 统一管理多个吊舱实例，提供注册、查询、移除、列出等功能。
 * 上层调用入口，后续可扩展统一能力路由。
 */

#include "../registry/pod_registry.h"
#include "../pod/interface/i_pod.h"
#include "../common/pod_result.h"
#include <memory>
#include <string>
#include <vector>

namespace PodModule {

/**
 * @brief 吊舱管理器
 * 
 * 负责统一管理所有吊舱设备：
 * - 添加/移除 Pod
 * - 查询 Pod
 * - 列出所有 Pod
 * - 后续可扩展统一能力路由
 */
class PodManager {
public:
    PodManager();
    ~PodManager();

    /**
     * @brief 添加一个吊舱
     * @param pod 吊舱实例
     * @return 操作结果
     */
    PodResult<void> addPod(std::shared_ptr<IPod> pod);

    /**
     * @brief 移除一个吊舱
     * @param pod_id 吊舱唯一标识
     * @return 操作结果
     */
    PodResult<void> removePod(const std::string& pod_id);

    /**
     * @brief 查询一个吊舱
     * @param pod_id 吊舱唯一标识
     * @return 吊舱实例，不存在返回 nullptr
     */
    std::shared_ptr<IPod> getPod(const std::string& pod_id) const;

    /**
     * @brief 列出所有吊舱
     * @return 所有吊舱实例列表
     */
    std::vector<std::shared_ptr<IPod>> listPods() const;

    /**
     * @brief 列出所有吊舱ID
     */
    std::vector<std::string> listPodIds() const;

    /**
     * @brief 获取管理的吊舱数量
     */
    size_t size() const;

private:
    PodRegistry registry_;  // 内部使用 PodRegistry 管理
};

} // namespace PodModule
