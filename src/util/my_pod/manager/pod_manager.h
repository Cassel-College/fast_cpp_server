#pragma once

/**
 * @file pod_manager.h
 * @brief 吊舱管理器（单例）
 * 
 * 统一管理多个吊舱实例，提供注册、查询、移除、列出等功能。
 * 通过 Init(json) 从配置初始化吊舱设备，由 Pipeline 调用启动。
 */

#include "../registry/pod_registry.h"
#include "../pod/interface/i_pod.h"
#include "../common/pod_result.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace PodModule {

/**
 * @brief 吊舱管理器（单例模式）
 * 
 * 负责统一管理所有吊舱设备：
 * - 从 JSON 配置初始化
 * - 添加/移除 Pod
 * - 查询 Pod
 * - 列出所有 Pod
 * - 获取状态快照（供 API 层使用）
 */
class PodManager {
public:
    static PodManager& GetInstance() {
        static PodManager instance;
        return instance;
    }

    PodManager(const PodManager&) = delete;
    PodManager& operator=(const PodManager&) = delete;

    /**
     * @brief 从 JSON 配置初始化吊舱管理器
     * @param config JSON 配置（包含 pod_args 等）
     * @return 是否初始化成功
     */
    bool Init(const nlohmann::json& config);

    /** @brief 是否已初始化 */
    bool IsInitialized() const { return initialized_.load(); }

    /** @brief 获取初始化时的配置 */
    nlohmann::json GetInitConfig() const;

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

    /**
     * @brief 获取所有吊舱的状态快照（JSON）
     * 供 API 层调用，返回全部吊舱的摘要信息
     */
    nlohmann::json GetStatusSnapshot() const;

    /**
     * @brief 连接指定吊舱
     */
    PodResult<void> connectPod(const std::string& pod_id);

    /**
     * @brief 断开指定吊舱
     */
    PodResult<void> disconnectPod(const std::string& pod_id);

private:
    PodManager();
    ~PodManager();

    /** @brief 根据厂商类型和配置创建 Pod 实例 */
    std::shared_ptr<IPod> createPod(const std::string& pod_id, const nlohmann::json& pod_cfg);

    PodRegistry registry_;
    nlohmann::json init_config_;
    mutable std::mutex mutex_;
    std::atomic<bool> initialized_{false};
};

} // namespace PodModule
