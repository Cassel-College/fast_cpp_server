/**
 * @file MyCacheProvider.cpp
 * @brief MyCache 单例包装器 —— 实现文件
 */

#include "MyCacheProvider.h"
#include "MyLog.h"

namespace my_cache {

// ============================================================================
// MyCacheProvider 静态成员初始化
// ============================================================================

std::atomic<bool> MyCacheProvider::initialized_flag_{false};
std::unique_ptr<MyCache> MyCacheProvider::instance_ = nullptr;
std::mutex MyCacheProvider::mutex_;

// ============================================================================
// MyCacheProvider 接口实现
// ============================================================================

CacheResult<void> MyCacheProvider::Init(const std::string& config_json) {
    MYLOG_INFO("[MyCacheProvider] Init 被调用，配置 JSON：{}", config_json);

    std::lock_guard lock(mutex_);

    // 如果已经初始化过且实例有效，直接返回成功
    if (initialized_flag_.load() && instance_ && instance_->Status() == CacheStatus::Running) {
        MYLOG_WARN("[MyCacheProvider] 已经初始化过，忽略重复调用");
        return CacheResult<void>::Success();
    }

    // 创建新实例并调用 Init
    instance_ = std::make_unique<MyCache>();
    auto result = instance_->Init(config_json);
    if (!result.Ok()) {
        MYLOG_ERROR("[MyCacheProvider] MyCache 初始化失败");
        instance_.reset();
        return result;
    }

    initialized_flag_.store(true);
    MYLOG_INFO("[MyCacheProvider] 初始化成功");
    return CacheResult<void>::Success();
}

CacheResult<MyCache*> MyCacheProvider::Get() {
    std::lock_guard lock(mutex_);
    if (!initialized_flag_.load() || !instance_ || instance_->Status() != CacheStatus::Running) {
        MYLOG_WARN("[MyCacheProvider] Get() 调用时模块未初始化");
        return CacheResult<MyCache*>::Fail(CacheErrorCode::NotInitialized, nullptr);
    }
    return CacheResult<MyCache*>::Success(instance_.get());
}

void MyCacheProvider::Destroy() {
    MYLOG_INFO("[MyCacheProvider] Destroy 被调用，销毁单例实例");
    std::lock_guard lock(mutex_);
    instance_.reset();
    initialized_flag_.store(false);
}

}  // namespace my_cache
