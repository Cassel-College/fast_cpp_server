#include "MyEdges.h"
#include "MyLog.h"
#include <algorithm>

namespace my_edge {

MyEdges& MyEdges::GetInstance() {
    try {
        static MyEdges instance;
        return instance;
    } catch (const std::exception& e) {
        MyLog::Error("获取 MyEdges 单例实例时发生异常: " + std::string(e.what()));
        throw;  // 重新抛出
    }
}

bool MyEdges::appendEdge(std::unique_ptr<IEdge> edge_ptr) {
    try {
        if (!edge_ptr) {
            MyLog::Error("尝试添加空 Edge 指针。");
            return false;
        }

        std::string edge_id = edge_ptr->Id();
        std::lock_guard<std::mutex> lock(mutex_);

        if (edges_.find(edge_id) != edges_.end()) {
            MyLog::Warn("ID 为 '" + edge_id + "' 的 Edge 已存在。");
            throw DuplicateEdgeException(edge_id);
        }

        edges_[edge_id] = std::move(edge_ptr);
        MyLog::Info("ID 为 '" + edge_id + "' 的 Edge 添加成功。");
        return true;
    } catch (const DuplicateEdgeException&) {
        throw;  // 重新抛出自定义异常
    } catch (const std::exception& e) {
        MyLog::Error("添加 Edge 时发生异常: " + std::string(e.what()));
        return false;
    }
}

const std::vector<const IEdge*>& MyEdges::getEdges() const {
    try {
        static thread_local std::vector<const IEdge*> temp_ptrs;
        temp_ptrs.clear();
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            temp_ptrs.push_back(pair.second.get());
        }
        return temp_ptrs;
    } catch (const std::exception& e) {
        MyLog::Error("获取所有 Edges 时发生异��: " + std::string(e.what()));
        static const std::vector<const IEdge*> empty_vec;  // 返回空向量
        return empty_vec;
    }
}

const std::vector<std::string>& MyEdges::getEdgeIds() const {
    try {
        static thread_local std::vector<std::string> temp_ids;
        temp_ids.clear();
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            temp_ids.push_back(pair.first);
        }
        return temp_ids;
    } catch (const std::exception& e) {
        MyLog::Error("获取所有 Edge IDs 时发生异常: " + std::string(e.what()));
        static const std::vector<std::string> empty_vec;  // 返回空向量
        return empty_vec;
    }
}

bool MyEdges::deleteEdgeById(const std::string& edge_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MyLog::Warn("尝试删除不存在的 Edge，ID 为 '" + edge_id + "'。");
            return false;
        }
        edges_.erase(it);
        MyLog::Info("ID 为 '" + edge_id + "' 的 Edge 删除成功。");
        return true;
    } catch (const std::exception& e) {
        MyLog::Error("删除 Edge 时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool MyEdges::startAllEdges() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        bool all_success = true;
        for (auto& pair : edges_) {
            std::string error_msg;
            try {
                if (!pair.second->Start(&error_msg)) {
                    MyLog::Error("启动 ID 为 '" + pair.first + "' 的 Edge 失败。错误信息: " + error_msg);
                    all_success = false;
                } else {
                    MyLog::Info("ID 为 '" + pair.first + "' 的 Edge 启动成功。");
                }
            } catch (const std::exception& e) {
                MyLog::Error("启动 Edge '" + pair.first + "' 时发生异常: " + e.what());
                all_success = false;
            }
        }
        return all_success;
    } catch (const std::exception& e) {
        MyLog::Error("启动所有 Edges 时发生异常: " + std::string(e.what()));
        return false;
    }
}

const std::unique_ptr<IEdge>& MyEdges::getEdgeById(const std::string& edge_id) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = edges_.find(edge_id);
        if (it == edges_.end()) {
            MyLog::Warn("ID 为 '" + edge_id + "' 的 Edge 未找到。");
            throw EdgeNotFoundException(edge_id);
        }
        return it->second;
    } catch (const EdgeNotFoundException&) {
        throw;  // 重新抛出
    } catch (const std::exception& e) {
        MyLog::Error("获取 Edge 时发生异常: " + std::string(e.what()));
        throw EdgeNotFoundException(edge_id);  // 或处理为默认
    }
}

nlohmann::json MyEdges::GetHeartbeatInfo() const {
    nlohmann::json heartbeat_info = nlohmann::json::object();
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : edges_) {
            heartbeat_info[pair.first] = pair.second->GetStatusSnapshot().toJson();
        }
    } catch (const std::exception& e) {
        MyLog::Error("获取 Heartbeat 信息时发生异常: " + std::string(e.what()));
    }
    return heartbeat_info;
}

}  // namespace my_edge