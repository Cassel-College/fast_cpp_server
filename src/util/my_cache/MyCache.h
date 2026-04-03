#pragma once

/**
 * @file MyCache.h
 * @brief 本地文件缓存管理模块
 *
 * 功能概述：
 *   - 管理本地文件系统中的一个缓存目录
 *   - 默认构造，通过 Init(JSON) 传入配置后启动
 *   - 后台线程定期扫描目录，维护内存文件索引，使 Exists 查询达到 O(1) 复杂度
 *   - 支持配置最大文件大小、最长保留时间
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
#include <unordered_map>
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
 *   4. 可根据配置进行文件大小限制和过期清理
 *
 * 使用流程：
 *   MyCache cache;
 *   auto r = cache.Init(R"({"root_path":"/data/cache","max_file_size":10485760})");
 */
class MyCache {
public:
    /**
     * @brief 默认构造函数，不执行任何初始化
     *
     * 构造后 Status() 返回 CacheStatus::NotInitialized，
     * 必须调用 Init() 后才能使用其他接口。
     */
    MyCache();

    /// 析构函数，停止后台扫描线程
    ~MyCache();

    // 禁止拷贝和移动
    MyCache(const MyCache&) = delete;
    MyCache& operator=(const MyCache&) = delete;
    MyCache(MyCache&&) = delete;
    MyCache& operator=(MyCache&&) = delete;

    // ---- 初始化 ----

    /**
     * @brief 初始化缓存模块
     *
     * @param config_json JSON 格式的配置字符串，字段说明：
     *   - root_path (string, 必填): 缓存根目录路径，不存在时自动创建
     *   - max_file_size (uint64, 可选): 单个文件最大大小（字节），0 或缺省表示不限制
     *   - max_retention_seconds (int64, 可选): 文件最长保留时间（秒），0 或缺省表示不限制，-1 表示永久有效
     *
     * @return CacheResult<void> 初始化结果
     */
    CacheResult<void> Init(const std::string& config_json);

    // ---- 公共接口 ----

    /**
     * @brief 查询模块运行状态
     * @return CacheStatus 枚举值
     */
    CacheStatus Status() const;

    /**
     * @brief 保存文件到缓存目录
     * @param name 文件相对路径名（如 "a/b.txt"），支持子目录
     * @param data 文件二进制内容
     * @return CacheResult<void>，若超出 max_file_size 返回 FileTooLarge
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

    /**
     * @brief 获取当前配置（只读）
     */
    const CacheConfig& GetConfig() const;

    /**
     * @brief 获取缓存中所有文件的元信息列表
     * @return CacheResult<std::vector<FileInfo>> 成功时 value 为文件元信息列表
     */
    CacheResult<std::vector<FileInfo>> GetAllFileList();

    /**
     * @brief 获取指定目录下的文件元信息列表
     * @param folder_path 目标目录，支持空字符串（表示缓存根目录）、相对路径或位于缓存根目录内的绝对路径
     * @return CacheResult<std::vector<FileInfo>> 成功时 value 为文件元信息列表
     */
    CacheResult<std::vector<FileInfo>> GetFileList(const std::string& folder_path = "");

    /**
     * @brief 在指定目录下创建子目录
     * @param folder_path 父目录，支持空字符串（表示缓存根目录）、相对路径或位于缓存根目录内的绝对路径
     * @param new_folder_name 新目录名，仅允许单层目录名
     * @return CacheResult<std::string> 成功时 value 为新目录的绝对路径
     */
    CacheResult<std::string> CreateSubdirectory(const std::string& folder_path,
                                                const std::string& new_folder_name);

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

    /**
     * @brief 校验目录路径是否位于 root_path 范围内，支持相对/绝对路径
     * @param folder_path 用户传入的目录路径，空字符串表示根目录
     * @param[out] full_path 输出经过规范化处理的目录完整路径
     * @param allow_root 是否允许根目录本身
     * @return CacheErrorCode::Ok 表示安全
     */
    CacheErrorCode ValidateDirectoryPath(const std::string& folder_path,
                                         std::filesystem::path& full_path,
                                         bool allow_root = true) const;

    // ---- 后台扫描线程 ----

    /// 后台线程入口函数
    void ScanThreadFunc();

    /// 执行一次目录扫描，更新内存索引（含文件元信息）
    void RefreshIndex();

    /// 清理过期文件（仅当 max_retention_seconds > 0 时生效）
    void CleanExpiredFiles();

private:
    CacheConfig config_;                       // 配置信息
    std::filesystem::path root_path_;          // 缓存根目录（规范化后的绝对路径）
    std::atomic<CacheStatus> status_{CacheStatus::NotInitialized}; // 模块状态
    std::atomic<bool> running_{false};         // 后台线程运行标记

    mutable std::shared_mutex index_mutex_;    // 保护文件索引的读写锁
    std::unordered_map<std::string, FileInfo> file_index_; // 内存文件索引（相对路径→元信息）

    std::thread scan_thread_;                  // 后台扫描线程
    std::mutex cv_mutex_;                      // 条件变量互斥锁
    std::condition_variable cv_;               // 用于优雅退出的条件变量

    static constexpr int kScanIntervalSec = 5; // 扫描间隔（秒）
};

}  // namespace my_cache
