#include "MyEdgeManager.h"
#include "MyLog.h"
#include "demo/Task.h"
#include <algorithm>

namespace my_edge {

MyEdgeManager& MyEdgeManager::GetInstance() {
    try {
        static MyEdgeManager instance;
        return instance;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 MyEdgeManager 单例实例时发生异常: " + std::string(e.what()));
        throw;  // 重新抛出
    }
}

bool MyEdgeManager::appendEdge(std::unique_ptr<IEdge> edge_ptr) {
    try {
        if (!edge_ptr) {
            MYLOG_ERROR("尝试添加空 Edge 指针。");
            return false;
        }

        std::string edge_id = edge_ptr->Id();
        std::lock_guard<std::mutex> lock(mutex_);

        if (edges_.find(edge_id) != edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 已存在。");
            throw DuplicateEdgeException(edge_id);
        }

        edges_[edge_id] = std::move(edge_ptr);
        MYLOG_INFO("ID 为 '" + edge_id + "' 的 Edge 添加成功。");
        return true;
    } catch (const DuplicateEdgeException&) {
        throw;  // 重新抛出自定义异常
    } catch (const std::exception& e) {
        MYLOG_ERROR("添加 Edge 时发生异常: " + std::string(e.what()));
        return false;
    }
}

const std::vector<const IEdge*>& MyEdgeManager::getEdges() const {
    try {
        static thread_local std::vector<const IEdge*> temp_ptrs;
        temp_ptrs.clear();
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            temp_ptrs.push_back(pair.second.get());
        }
        return temp_ptrs;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取所有 Edges 时发生异��: " + std::string(e.what()));
        static const std::vector<const IEdge*> empty_vec;  // 返回空向量
        return empty_vec;
    }
}

const std::vector<std::string>& MyEdgeManager::getEdgeIds() const {
    try {
        static thread_local std::vector<std::string> temp_ids;
        temp_ids.clear();
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            temp_ids.push_back(pair.first);
        }
        return temp_ids;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取所有 Edge IDs 时发生异常: " + std::string(e.what()));
        static const std::vector<std::string> empty_vec;  // 返回空向量
        return empty_vec;
    }
}

bool MyEdgeManager::deleteEdgeById(const std::string& edge_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("尝试删除不存在的 Edge，ID 为 '" + edge_id + "'。");
            return false;
        }
        edges_.erase(it);
        MYLOG_INFO("ID 为 '" + edge_id + "' 的 Edge 删除成功。");
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("删除 Edge 时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::startAllEdges() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        bool all_success = true;
        for (auto& pair : edges_) {
            std::string error_msg;
            try {
                if (!pair.second->Start(&error_msg)) {
                    MYLOG_ERROR("启动 ID 为 '" + pair.first + "' 的 Edge 失败。错误信息: " + error_msg);
                    all_success = false;
                } else {
                    MYLOG_INFO("ID 为 '" + pair.first + "' 的 Edge 启动成功。");
                }
            } catch (const std::exception& e) {
                MYLOG_ERROR("启动 Edge '" + pair.first + "' 时发生异常: " + e.what());
                all_success = false;
            }
        }
        return all_success;
    } catch (const std::exception& e) {
        MYLOG_ERROR("启动所有 Edges 时发生异常: " + std::string(e.what()));
        return false;
    }
}

const std::unique_ptr<IEdge>& MyEdgeManager::getEdgeById(const std::string& edge_id) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到。");
        } else {
            MYLOG_INFO("ID 为 '" + edge_id + "' 的 Edge 获取成功。");
            return it->second;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Edge 时发生异常: " + std::string(e.what()));
    }
    return nullptr;  // 永远不会到达
}

nlohmann::json MyEdgeManager::GetHeartbeatInfo() const {
    nlohmann::json heartbeat_info = nlohmann::json::object();
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            heartbeat_info[pair.first] = pair.second->GetStatusSnapshot().toJson();
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Heartbeat 信息时发生异常: " + std::string(e.what()));
    }
    return heartbeat_info;
}

bool MyEdgeManager::HasEdge(const std::string& edge_id) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return edges_.find(edge_id) != edges_.end();
    } catch (const std::exception& e) {
        MYLOG_ERROR("检查 Edge 存在性时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::IsEmpty() const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return edges_.empty();
    } catch (const std::exception& e) {
        MYLOG_ERROR("检查 Edge 集合是否为空时发生异常: " + std::string(e.what()));
        return true;  // 出错时假设为空
    }
}

bool MyEdgeManager::SelectEdgeByIdDoAction(const std::string& edge_id, const nlohmann::json& action) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法执行操作。");
            return false;
        }
        bool result = false;
        // return it->second->ExecuteAction(action);
        return result;
    } catch (const std::exception& e) {
        MYLOG_ERROR("对 Edge 执行操作时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::appendTaskToEdgeById(const std::string& edge_id, const nlohmann::json& task) const {
    MYLOG_INFO("尝试向 ID 为 '{}' 的 Edge 添加任务: {}", edge_id, task.dump(4));
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法添加任务。");
            return false;
        } else {
            MYLOG_INFO("找到 ID 为 '{}' 的 Edge，准备添加任务。", edge_id);
        }
        bool result = false;
        result = it->second->AppendJsonTask(task);
        return result;
    } catch (const std::exception& e) {
        MYLOG_ERROR("向 Edge 添加任务时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::appendTaskToEdgeByIdV2(const std::string& edge_id, const my_data::Task& task) const {
    MYLOG_INFO("尝试向 ID 为 '{}' 的 Edge 添加任务: {}", edge_id, task.toString());
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法添加任务。");
            return false;
        } else {
            MYLOG_INFO("找到 ID 为 '{}' 的 Edge，准备添加任务。", edge_id);
        }
        bool result = false;
        result = it->second->AppendTask(task);
        return result;
    } catch (const std::exception& e) {
        MYLOG_ERROR("向 Edge 添加任务时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::setESTOP(const std::string& edge_id, bool estop) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法设置 ESTOP。");
            return false;
        }
        bool result = false;
        // return it->second->SetESTOP(estop);
        return result;
    } catch (const std::exception& e) {
        MYLOG_ERROR("设置 Edge ESTOP 时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::setAllEdgesESTOP(bool estop) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        bool all_success = true;
        for (auto& pair : edges_) {
            try {
                // if (!pair.second->SetESTOP(estop)) {
                //     MYLOG_ERROR("设置 ID 为 '" + pair.first + "' 的 Edge ESTOP 失败。");
                //     all_success = false;
                // } else {
                //     MYLOG_INFO("ID 为 '" + pair.first + "' 的 Edge ESTOP 设置成功。");
                // }
            } catch (const std::exception& e) {
                MYLOG_ERROR("设置 Edge '" + pair.first + "' ESTOP 时发生异常: " + e.what());
                all_success = false;
            }
        }
        return all_success;
    } catch (const std::exception& e) {
        MYLOG_ERROR("设置所有 Edges ESTOP 时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::getEdgeOnlineStatus(const std::string& edge_id) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法获取在线状态。");
            return false;
        }
        bool result = false;
        // return it->second->IsOnline();
        return result;  // 占位符
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Edge 在线状态时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::getAllEdgesOnlineStatus(std::unordered_map<std::string, bool>& status_map) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            // status_map[pair.first] = pair.second->IsOnline();
        }
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取所有 Edges 在线状态时发生异常: " + std::string(e.what()));
        return false;
    }
}
 

bool MyEdgeManager::getOnlineEdges(std::vector<std::string>& online_edges) const {
    MYLOG_INFO("获取在线 Edges 列表...");
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        online_edges.clear();
        for (const auto& pair : edges_) {
            // if (pair.second->IsOnline()) {
            //     online_edges.push_back(pair.first);
            // }
            online_edges.push_back(pair.first);  // 占位符
        }
        MYLOG_INFO("在线 Edges 列表获取完成，数量: " + std::to_string(online_edges.size()));
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取在线 Edges 列表时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::getEdgeTaskStatus(const std::string& edge_id, nlohmann::json& task_status) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法获取任务状态。");
            return false;
        }
        // task_status = it->second->GetTaskStatus();
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Edge 任务状态时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::getEdgeHistoryTaskStatus(const std::string& edge_id, nlohmann::json& history_task_status) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法获取历史任务状态。");
            return false;
        }
        // history_task_status = it->second->GetHistoryTaskStatus();
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Edge 历史任务状态时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdgeManager::getEdgeInternalDumpInfo(const std::string& edge_id, nlohmann::json& dump_info) const {
    bool foundStatus = false;
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法获取内部信息 Dump。");
            foundStatus = false;
        } else {
            dump_info = it->second->DumpInternalInfo();
            foundStatus = true;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Edge 内部信息 Dump 时发生异常: " + std::string(e.what()));
        foundStatus = false;
    }
    return foundStatus;
}

bool MyEdgeManager::getEdgeRunTimeStatusInfo(const std::string& edge_id, nlohmann::json& status_info) const {
    bool foundStatus = false;
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MYLOG_WARN("ID 为 '" + edge_id + "' 的 Edge 未找到，无法获取运行时状态信息。");
            foundStatus = false;
        } else {
            status_info = it->second->GetRunTimeStatusInfo();
            foundStatus = true;
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("获取 Edge 运行时状态信息时发生异常: " + std::string(e.what()));
        foundStatus = false;
    }
    return foundStatus;
}

}  // namespace my_edge