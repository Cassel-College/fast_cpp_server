/**
 * @file MyContext.cpp
 * @brief MyContext 运行时上下文单例实现
 */

#include "MyContext.h"

#include <algorithm>
#include <chrono>
#include <vector>

#include "MyLog.h"

namespace my_comm {

// ============================================================================
// ContextEntry
// ============================================================================

nlohmann::json ContextEntry::ToJson() const {
    nlohmann::json j;
    try {
        j["name"] = name;
        j["value"] = value;
        j["description"] = description;
        j["type"] = MyContext::TypeToString(type);
        j["created_at_ms"] = created_at_ms;
        j["updated_at_ms"] = updated_at_ms;
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] ContextEntry::ToJson 序列化失败: {}", e.what());
    }
    return j;
}

// ============================================================================
// 单例
// ============================================================================

MyContext& MyContext::GetInstance() {
    static MyContext instance;
    return instance;
}

// ============================================================================
// 内部辅助
// ============================================================================

int64_t MyContext::NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

ContextValueType MyContext::DeduceType(const nlohmann::json& value) {
    if (value.is_boolean()) return ContextValueType::Boolean;
    if (value.is_number_integer() || value.is_number_unsigned()) return ContextValueType::Integer;
    if (value.is_number_float()) return ContextValueType::Double;
    if (value.is_string()) return ContextValueType::String;
    if (value.is_object()) return ContextValueType::Object;
    if (value.is_array()) return ContextValueType::Array;
    return ContextValueType::Null;
}

const char* MyContext::TypeToString(ContextValueType type) {
    switch (type) {
        case ContextValueType::Boolean: return "boolean";
        case ContextValueType::Integer: return "integer";
        case ContextValueType::Double:  return "double";
        case ContextValueType::String:  return "string";
        case ContextValueType::Object:  return "object";
        case ContextValueType::Array:   return "array";
        case ContextValueType::Null:
        default:                        return "null";
    }
}

bool MyContext::SetLocked(const std::string& name, const nlohmann::json& value,
                          const std::string& description) {
    const int64_t now = NowMs();
    auto it = entries_.find(name);
    if (it == entries_.end()) {
        ContextEntry entry;
        entry.name = name;
        entry.value = value;
        entry.description = description;
        entry.type = DeduceType(value);
        entry.created_at_ms = now;
        entry.updated_at_ms = now;
        entries_.emplace(name, std::move(entry));
        MYLOG_INFO("[MyContext] 新增上下文: {} (type={})", name, TypeToString(DeduceType(value)));
    } else {
        it->second.value = value;
        it->second.type = DeduceType(value);
        if (!description.empty()) {
            it->second.description = description;
        }
        it->second.updated_at_ms = now;
        MYLOG_INFO("[MyContext] 更新上下文: {} (type={})", name, TypeToString(it->second.type));
    }
    return true;
}

// ============================================================================
// 写入 / 注册
// ============================================================================

bool MyContext::Set(const std::string& name, const nlohmann::json& value,
                    const std::string& description) {
    if (name.empty()) {
        MYLOG_WARN("[MyContext] Set 失败: name 不能为空");
        return false;
    }
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return SetLocked(name, value, description);
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] Set 异常: {}", e.what());
        return false;
    }
}

bool MyContext::SetBool(const std::string& name, bool value, const std::string& description) {
    return Set(name, nlohmann::json(value), description);
}

bool MyContext::SetInt(const std::string& name, int64_t value, const std::string& description) {
    return Set(name, nlohmann::json(value), description);
}

bool MyContext::SetDouble(const std::string& name, double value, const std::string& description) {
    return Set(name, nlohmann::json(value), description);
}

bool MyContext::SetString(const std::string& name, const std::string& value,
                          const std::string& description) {
    return Set(name, nlohmann::json(value), description);
}

// ============================================================================
// 修改（要求已存在）
// ============================================================================

bool MyContext::UpdateValue(const std::string& name, const nlohmann::json& value,
                            std::string& error_out) {
    if (name.empty()) {
        error_out = "name 不能为空";
        return false;
    }
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(name);
        if (it == entries_.end()) {
            error_out = "上下文不存在: " + name;
            return false;
        }
        it->second.value = value;
        it->second.type = DeduceType(value);
        it->second.updated_at_ms = NowMs();
        MYLOG_INFO("[MyContext] 修改上下文值: {} (type={})", name, TypeToString(it->second.type));
        return true;
    } catch (const std::exception& e) {
        error_out = e.what();
        MYLOG_ERROR("[MyContext] UpdateValue 异常: {}", e.what());
        return false;
    }
}

bool MyContext::UpdateDescription(const std::string& name, const std::string& description,
                                  std::string& error_out) {
    if (name.empty()) {
        error_out = "name 不能为空";
        return false;
    }
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(name);
        if (it == entries_.end()) {
            error_out = "上下文不存在: " + name;
            return false;
        }
        it->second.description = description;
        it->second.updated_at_ms = NowMs();
        return true;
    } catch (const std::exception& e) {
        error_out = e.what();
        MYLOG_ERROR("[MyContext] UpdateDescription 异常: {}", e.what());
        return false;
    }
}

// ============================================================================
// 读取
// ============================================================================

bool MyContext::Has(const std::string& name) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.find(name) != entries_.end();
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] Has 异常: {}", e.what());
        return false;
    }
}

bool MyContext::GetEntry(const std::string& name, ContextEntry& out) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(name);
        if (it == entries_.end()) return false;
        out = it->second;
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] GetEntry 异常: {}", e.what());
        return false;
    }
}

bool MyContext::GetValue(const std::string& name, nlohmann::json& out) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(name);
        if (it == entries_.end()) return false;
        out = it->second.value;
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] GetValue 异常: {}", e.what());
        return false;
    }
}

bool MyContext::GetBool(const std::string& name, bool default_value) const {
    nlohmann::json v;
    if (!GetValue(name, v)) return default_value;
    try {
        if (v.is_boolean()) return v.get<bool>();
        if (v.is_number()) return v.get<double>() != 0.0;
        if (v.is_string()) {
            std::string s = v.get<std::string>();
            return s == "true" || s == "1";
        }
    } catch (const std::exception& e) {
        MYLOG_WARN("[MyContext] GetBool 转换失败({}): {}", name, e.what());
    }
    return default_value;
}

int64_t MyContext::GetInt(const std::string& name, int64_t default_value) const {
    nlohmann::json v;
    if (!GetValue(name, v)) return default_value;
    try {
        if (v.is_number()) return static_cast<int64_t>(v.get<double>());
        if (v.is_boolean()) return v.get<bool>() ? 1 : 0;
        if (v.is_string()) return static_cast<int64_t>(std::stoll(v.get<std::string>()));
    } catch (const std::exception& e) {
        MYLOG_WARN("[MyContext] GetInt 转换失败({}): {}", name, e.what());
    }
    return default_value;
}

double MyContext::GetDouble(const std::string& name, double default_value) const {
    nlohmann::json v;
    if (!GetValue(name, v)) return default_value;
    try {
        if (v.is_number()) return v.get<double>();
        if (v.is_boolean()) return v.get<bool>() ? 1.0 : 0.0;
        if (v.is_string()) return std::stod(v.get<std::string>());
    } catch (const std::exception& e) {
        MYLOG_WARN("[MyContext] GetDouble 转换失败({}): {}", name, e.what());
    }
    return default_value;
}

std::string MyContext::GetString(const std::string& name, const std::string& default_value) const {
    nlohmann::json v;
    if (!GetValue(name, v)) return default_value;
    try {
        if (v.is_string()) return v.get<std::string>();
        return v.dump();
    } catch (const std::exception& e) {
        MYLOG_WARN("[MyContext] GetString 转换失败({}): {}", name, e.what());
    }
    return default_value;
}

// ============================================================================
// 删除 / 清空 / 统计
// ============================================================================

bool MyContext::Erase(const std::string& name) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        const bool erased = entries_.erase(name) > 0;
        if (erased) {
            MYLOG_INFO("[MyContext] 删除上下文: {}", name);
        }
        return erased;
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] Erase 异常: {}", e.what());
        return false;
    }
}

void MyContext::Clear() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        MYLOG_INFO("[MyContext] 已清空全部上下文");
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] Clear 异常: {}", e.what());
    }
}

std::size_t MyContext::Size() const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] Size 异常: {}", e.what());
        return 0;
    }
}

// ============================================================================
// 序列化
// ============================================================================

nlohmann::json MyContext::GetAllJson() const {
    nlohmann::json items = nlohmann::json::array();
    try {
        std::vector<ContextEntry> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot.reserve(entries_.size());
            for (const auto& kv : entries_) {
                snapshot.push_back(kv.second);
            }
        }
        std::sort(snapshot.begin(), snapshot.end(),
                  [](const ContextEntry& a, const ContextEntry& b) { return a.name < b.name; });
        for (const auto& entry : snapshot) {
            items.push_back(entry.ToJson());
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("[MyContext] GetAllJson 异常: {}", e.what());
    }
    nlohmann::json out;
    out["count"] = items.size();
    out["items"] = items;
    return out;
}

bool MyContext::GetEntryJson(const std::string& name, nlohmann::json& out) const {
    ContextEntry entry;
    if (!GetEntry(name, entry)) {
        return false;
    }
    out = entry.ToJson();
    return true;
}

}  // namespace my_comm
