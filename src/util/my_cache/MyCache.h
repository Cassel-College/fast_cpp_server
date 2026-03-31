#pragma once

/**
 * @file MyCache.h
 * @brief 本地文件缓存管理模块
 *
 * 功能概述：
 *   - 管理本地文件系统中的一个缓存目录
 *   - 后台线程定期扫描目录，维护内存文件索引，使 Exists 查询达到 O(1) 复杂度
 *   - 所有操作均防止路径穿越攻击
 *   - MyCacheProvider 提供线程安全的单例包装
 *
 * 错误处理：
 *   - 所有接口返回 CacheResult<T>，包含错误码和可选值
 */

#include "CacheTypes.h"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace my_cache {

// ============================================================================
// MyCache 核心类
// ============================================================================

/**
 * @brief 本地文件缓存管理器
 *
 * 职责：
 *   1. 管理 root_path 下的文件存取
 *   2. 后台线程每 5 秒扫描一次目录，维护内存文件索引
 *   3. 所有公开接口均做路径穿越校验
 */
class MyCache {
public:
    /**
     * @brief 构造函数
     * @param root_path 缓存根目录路径，不存在时自动创建
     */
    explicit MyCache(const std::string& root_path);

    /// 析构函数，停止后台扫描线程
    ~MyCache();

    // 禁止拷贝和移动
    MyCache(const MyCache&) = delete;
    MyCache& operator=(const MyCache&) = delete;
    MyCache(MyCache&&) = delete;
    MyCache& operator=(MyCache&&) = delete;

    // ---- 公共接口 ----

    /**
     * @brief 查询模块是否初始化成功
     * @return true 表示 root_path 有效且后台线程运行中
     */
    bool Status() const;

    /**
     * @brief 保存文件到缓存目录
     * @param name 文件相对路径名（如 "a/b.txt"），支持子目录
     * @param data 文件二进制内容
     * @return CacheResult<void>
     */
    CacheResult<void> SaveFile(const std::string& name,
                               const std::vector<uint8_t>& data);

    /**
     * @brief 删除缓存目录中的文件
     * @param name 文件相对路径名
     * @return CacheResult<void>
     */
    CacheResult<void> DeleteFile(const std::string& name);

    /**
     * @brief 获取文件的完整绝对路径
     * @param name 文件相对路径名
     * @return CacheResult<std::string> 成功时 value 为绝对路径
     */
    CacheResult<std::string> GetFullPath(const std::string& name);

    /**
     * @brief O(1) 查询文件是否存在（基于内存索引）
     * @param name 文件相对路径名
     * @return CacheResult<bool> 成功时 value 为 true/false
     */
    CacheResult<bool> Exists(const std::string& name);

    /**
     * @brief 获取当前缓存根目录路径
     */
    std::string GetRootPath() const;

private:
    // ---- 路径安全验证 ----

    /**
     * @brief 验证请求路径是否在 root_path 范围内，防止路径穿越
     * @param name 用户传入的相对路径
     * @param[out] full_path 输出经过规范化处理的完整路径
     * @return CacheErrorCode::Ok 表示安全
     */
    CacheErrorCode ValidatePath(const std::string& name,
                                std::filesystem::path& full_path) const;

    // ---- 后台扫描线程 ----

    /// 后台线程入口函数
    void ScanThreadFunc();

    /// 执行一次目录扫描，更新内存索引
    void RefreshIndex();

private:
    std::filesystem::path root_path_;          // 缓存根目录（规范化后的绝对路径）
    std::atomic<bool> initialized_{false};     // 初始化是否成功
    std::atomic<bool> running_{false};         // 后台线程运行标记

    mutable std::shared_mutex index_mutex_;    // 保护文件索引的读写锁
    std::unordered_set<std::string> file_index_; // 内存文件索引（相对路径集合）

    std::thread scan_thread_;                  // 后台扫描线程
    std::mutex cv_mutex_;                      // 条件变量互斥锁
    std::condition_variable cv_;               // 用于优雅退出的条件变量

    static constexpr int kScanIntervalSec = 5; // 扫描间隔（秒）
};

}  // namespace my_cache
