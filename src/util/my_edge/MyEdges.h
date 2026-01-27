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

    // /**
    //  * @brief 检查是否存在指定 ID 的 Edge。
    //  * @param edge_id Edge 的 ID。
    //  * @return 如果存在则返回 true，否则返回 false。
    //  */
    // bool HasEdge(const std::string& edge_id) const;

    // /**
    //  * @brief 检查集合是否为空。
    //  * @return 如果为空则返回 true，否则返回 false。
    //  */
    // bool IsEmpty() const;

    // /** 
    //  * @brief 根据 ID 选择 Edge 并执行操作。
    //  * @param edge_id Edge 的 ID。
    //  * @param action 要执行的操作，使用 nlohmann::json 表示。
    //  * @return 如果操作成功执行则返回 true，否则返回 false。
    //  */
    // bool SelectEdgeByIdDoAction(const std::string& edge_id, const nlohmann::json& action) const;

    // /**
    //  * @brief 向指定 ID 的 Edge 添加任务。
    //  * @param edge_id Edge 的 ID。
    //  * @param task 要添加的任务，使用 nlohmann::json 表示。
    //  * @return 如果任务成功添加则返回 true，否则返回 false。
    //  */
    // bool appendTaskToEdgeById(const std::string& edge_id, const nlohmann::json& task) const;

    // /**
    //  * @brief 设置指定 ID 的 Edge 的紧急停止状态。
    //  * @param edge_id Edge 的 ID。
    //  * @param estop 紧急停止状态。
    //  * @return 如果成功设置则返回 true，否则返回 false。
    //  */
    // bool setESTOP(const std::string& edge_id, bool estop) const;

    // /**
    //  * @brief 设置所有 Edge 的紧急停止状态。
    //  * @param estop 紧急停止状态。
    //  * @return 如果成功设置则返回 true，否则返回 false。
    //  */
    // bool setAllEdgesESTOP(bool estop) const;

    // /**
    //  * @brief 获取指定 ID 的 Edge 的在线状态。
    //  * @param edge_id Edge 的 ID。
    //  * @return 如果在线则返回 true，否则返回 false。
    //  */
    // bool getEdgeOnlineStatus(const std::string& edge_id) const;

    // /**
    //  * @brief 获取所有 Edge 的在线状态。
    //  * @param status_map 用于存储 Edge ID 和其在线状态的映射。
    //  * @return 如果成功获取则返回 true，否则返回 false。
    //  */
    // bool getAllEdgesOnlineStatus(std::unordered_map<std::string, bool>& status_map) const;
    // /**
    //  * @brief 获取所有在线的 Edge ID 列表。
    //  * @param online_edges 用于存储在线 Edge ID 的向量。
    //  * @return 如果成功获取则返回 true，否则返回 false。
    //  */
    // bool getOnlineEdges(std::vector<std::string>& online_edges) const;

    // /**
    //  * @brief 获取指定 ID 的 Edge 的任务状态。
    //  * @param edge_id Edge 的 ID。
    //  * @param task_status 用于存储任务状态的 JSON 对象。
    //  * @return 如果成功获取则返回 true，否则返回 false。
    //  */
    // bool getEdgeTaskStatus(const std::string& edge_id, nlohmann::json& task_status) const;

    // /**
    //  * @brief 获取指定 ID 的 Edge 的历史任务状态。
    //  * @param edge_id Edge 的 ID。
    //  * @param history_task_status 用于存储历史任务状态的 JSON 对象。
    //  * @return 如果成功获取则返回 true，否则返回 false。
    //  */
    // bool getEdgeHistoryTaskStatus(const std::string& edge_id, nlohmann::json& history_task_status) const;

private:
    MyEdges() = default;
    ~MyEdges() = default;  // 析构函数，用于必要清理
    MyEdges(const MyEdges&) = delete;
    MyEdges& operator=(const MyEdges&) = delete;

    mutable std::mutex mutex_;  // 用于线程安全
    std::unordered_map<std::string, std::unique_ptr<IEdge>> edges_;  // 以 ID 为键的高效存储
};

}  // namespace my_edge