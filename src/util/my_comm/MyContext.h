#pragma once
#ifndef MY_COMM_MY_CONTEXT_H
#define MY_COMM_MY_CONTEXT_H

/**
 * @file MyContext.h
 * @brief 全项目共享的运行时上下文单例模块
 *
 * MyContext 提供一个进程内、线程安全的「运行时上下文 / 暂存标志」中心，
 * 用于存放程序运行期间大量的临时状态。每一条上下文记录（ContextEntry）包含：
 *   - name        变量名（唯一键）
 *   - value       变量值（可为 bool / 整数 / 浮点 / 字符串 / 对象 / 数组）
 *   - description  变量解释（用途说明）
 *   - type        值类型（由 value 自动推导）
 *   - created/updated_at_ms  创建/更新时间戳（毫秒）
 *
 * 值统一使用 nlohmann::json 承载，以支持多种数据类型；所有读写操作均加锁，
 * 可在多线程环境下安全使用。配套的 REST API 见
 * src/util/my_api/controller/context/ContextController。
 */

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace my_comm {

/**
 * @brief 上下文值的类型枚举（由 nlohmann::json 实际类型推导得到）
 */
enum class ContextValueType {
    Null,
    Boolean,
    Integer,
    Double,
    String,
    Object,
    Array
};

/**
 * @brief 单条运行时上下文记录
 */
struct ContextEntry {
    std::string name;                                ///< 变量名（唯一键）
    nlohmann::json value;                            ///< 变量值
    std::string description;                         ///< 变量解释
    ContextValueType type{ContextValueType::Null};   ///< 值类型
    int64_t created_at_ms{0};                        ///< 创建时间戳（毫秒）
    int64_t updated_at_ms{0};                        ///< 最近更新时间戳（毫秒）

    /// 序列化为 JSON 对象
    nlohmann::json ToJson() const;
};

/**
 * @brief 运行时上下文单例
 *
 * 线程安全：所有公共方法内部均通过互斥锁保护，可在任意线程调用。
 * 异常安全：所有方法均做了防御性处理，不会向外抛出异常导致进程崩溃。
 */
class MyContext {
public:
    /// 获取全局唯一实例
    static MyContext& GetInstance();

    MyContext(const MyContext&) = delete;
    MyContext& operator=(const MyContext&) = delete;
    MyContext(MyContext&&) = delete;
    MyContext& operator=(MyContext&&) = delete;

    // ------------------------------------------------------------------
    // 写入 / 注册（upsert 语义：不存在则创建，存在则更新值；
    // description 非空时同步更新解释，传空则保留原解释）
    // ------------------------------------------------------------------
    bool SetBool(const std::string& name, bool value, const std::string& description = "");
    bool SetInt(const std::string& name, int64_t value, const std::string& description = "");
    bool SetDouble(const std::string& name, double value, const std::string& description = "");
    bool SetString(const std::string& name, const std::string& value, const std::string& description = "");
    /// 通用写入，value 可为任意 JSON 类型
    bool Set(const std::string& name, const nlohmann::json& value, const std::string& description = "");

    // ------------------------------------------------------------------
    // 修改（要求记录已存在），用于 API「修改某个上下文」
    // ------------------------------------------------------------------
    /// 仅修改值，记录必须已存在；失败时通过 error_out 返回原因
    bool UpdateValue(const std::string& name, const nlohmann::json& value, std::string& error_out);
    /// 仅修改解释，记录必须已存在；失败时通过 error_out 返回原因
    bool UpdateDescription(const std::string& name, const std::string& description, std::string& error_out);

    // ------------------------------------------------------------------
    // 读取
    // ------------------------------------------------------------------
    bool Has(const std::string& name) const;
    bool GetEntry(const std::string& name, ContextEntry& out) const;
    bool GetValue(const std::string& name, nlohmann::json& out) const;

    bool GetBool(const std::string& name, bool default_value = false) const;
    int64_t GetInt(const std::string& name, int64_t default_value = 0) const;
    double GetDouble(const std::string& name, double default_value = 0.0) const;
    std::string GetString(const std::string& name, const std::string& default_value = "") const;

    // ------------------------------------------------------------------
    // 删除 / 清空 / 统计
    // ------------------------------------------------------------------
    bool Erase(const std::string& name);
    void Clear();
    std::size_t Size() const;

    // ------------------------------------------------------------------
    // 序列化（供 REST API 使用）
    // ------------------------------------------------------------------
    /// 返回 { "count": N, "items": [ entry, ... ] }，items 按 name 升序
    nlohmann::json GetAllJson() const;
    /// 返回单条记录的 JSON；不存在返回 false
    bool GetEntryJson(const std::string& name, nlohmann::json& out) const;

    /// 类型枚举转字符串
    static const char* TypeToString(ContextValueType type);

private:
    MyContext() = default;
    ~MyContext() = default;

    static ContextValueType DeduceType(const nlohmann::json& value);
    static int64_t NowMs();

    /// upsert 内部实现（调用者必须已持有 mutex_）
    bool SetLocked(const std::string& name, const nlohmann::json& value, const std::string& description);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ContextEntry> entries_;
};

}  // namespace my_comm

#endif  // MY_COMM_MY_CONTEXT_H
