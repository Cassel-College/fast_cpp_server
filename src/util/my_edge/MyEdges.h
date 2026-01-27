#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
#include "IEdge.h"
#include "MyLog.h"

namespace my_edge {

/**
 * @brief 当指定 ID 的 Edge 未找到时抛出的异常。
 */
class EdgeNotFoundException : public std::runtime_error {
public:
    explicit EdgeNotFoundException(const std::string& edge_id)
        : std::runtime_error("ID 为 '" + edge_id + "' 的 Edge 未找到。") {}
};

/**
 * @brief 尝试添加具有重复 ID 的 Edge 时抛出的异常。
 */
class DuplicateEdgeException : public std::runtime_error {
public:
    explicit DuplicateEdgeException(const std::string& edge_id)
        : std::runtime_error("ID 为 '" + edge_id + "' 的 Edge 已存在。") {}
};

/**
 * @brief MyEdges - 企业级单例 IEdge 实例管理器。
 * 
 * 此类提供线程安全的操作，用于管理 IEdge 对象的集合。
 * 使用 std::unique_ptr 进行独占所有权，使用 std::unordered_map 进行高效查找。
 */
class MyEdges {
public:
    /**
     * @brief 获取单例实例。
     * @return 单例 MyEdges 实例的引用。
     */
    static MyEdges& GetInstance();

    /**
     * @brief 将 Edge 实例添加到集合中。
     * @param edge_ptr Edge 实例的唯一指针。
     * @return 如果成功添加则返回 true，否则返回 false（例如 ID 重复）。
     * @throws DuplicateEdgeException 如果具有相同 ID 的 Edge 已存在。
     */
    bool appendEdge(std::unique_ptr<IEdge> edge_ptr);

    /**
     * @brief 获取所有 Edge 实例（只读）。
     * @return 唯一指针向量的常量引用。
     * 注意：实际实现返回常量指针向量以避免复制。
     */
    const std::vector<const IEdge*>& getEdges() const;

    /**
     * @brief 获取所有 Edge ID。
     * @return Edge ID 向量的常量引用。
     */
    const std::vector<std::string>& getEdgeIds() const;

    /**
     * @brief 通过 ID 删除 Edge。
     * @param edge_id 要删除的 Edge 的 ID。
     * @return 如果删除成功则返回 true，否则返回 false（未找到）。
     */
    bool deleteEdgeById(const std::string& edge_id);

    /**
     * @brief 启动所有 Edge 实例。
     * @return 如果全部启动成功则返回 true，否则返回 false。
     */
    bool startAllEdges();

    /**
     * @brief 通过 ID 获取 Edge（只读）。
     * @param edge_id Edge 的 ID。
     * @return 唯一指针的常量引用。
     * @throws EdgeNotFoundException 如果未找到。
     * 
     * 注意：返回常量引用，不会导致集合变动。
     */
    const std::unique_ptr<IEdge>& getEdgeById(const std::string& edge_id) const;

    /**
     * @brief Get the Heartbeat Info object/获取给模块的心跳信息
     * 
     * @return nlohmann::json 
     */
    nlohmann::json GetHeartbeatInfo() const;

private:
    MyEdges() = default;
    ~MyEdges() = default;  // 析构函数，用于必要清理
    MyEdges(const MyEdges&) = delete;
    MyEdges& operator=(const MyEdges&) = delete;

    mutable std::mutex mutex_;  // 用于线程安全
    std::unordered_map<std::string, std::unique_ptr<IEdge>> edges_;  // 以 ID 为键的高效存储
};

}  // namespace my_edge