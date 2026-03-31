#pragma once

/**
 * @file MyCacheProvider.h
 * @brief MyCache 单例包装器
 *
 * 提供线程安全的全局访问入口：
 *   1. 程序启动时调用 MyCacheProvider::Init("/path/to/cache")
 *   2. 之后通过 MyCacheProvider::Get() 获取 MyCache 实例
 */

#include "MyCache.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace my_cache {

/**
 * @brief MyCache 的单例包装器，提供线程安全的显式初始化
 *
 * 用法：
 *   1. 程序启动时调用 MyCacheProvider::Init("/path/to/cache")
 *   2. 之后通过 MyCacheProvider::Get() 获取 MyCache 实例
 */
class MyCacheProvider {
public:
    /**
     * @brief 初始化单例（线程安全，仅首次调用生效）
     * @param root_path 缓存根目录
     * @return CacheResult<void> 初始化结果
     */
    static CacheResult<void> Init(const std::string& root_path);

    /**
     * @brief 获取 MyCache 实例
     * @return CacheResult<MyCache*>，未初始化时返回 NotInitialized
     */
    static CacheResult<MyCache*> Get();

    /**
     * @brief 销毁单例实例（主要用于测试）
     */
    static void Destroy();

    // 禁止实例化
    MyCacheProvider() = delete;
    ~MyCacheProvider() = delete;

private:
    static std::atomic<bool> initialized_flag_;
    static std::unique_ptr<MyCache> instance_;
    static std::mutex mutex_;
};

}  // namespace my_cache
