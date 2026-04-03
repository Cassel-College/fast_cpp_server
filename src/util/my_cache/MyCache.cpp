/**
 * @file MyCache.cpp
 * @brief 本地文件缓存管理模块 —— 实现文件
 */

#include "MyCache.h"
#include "MyLog.h"

#include <nlohmann/json.hpp>
#include <ctime>
#include <fstream>

using json = nlohmann::json;

namespace my_cache {

// ============================================================================
// 工具函数（文件内部）
// ============================================================================

namespace {

/// 将 filesystem::file_time_type 转为 ISO 8601 字符串
std::string FormatFileTime(const std::filesystem::file_time_type& ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now()
             + std::chrono::system_clock::now()
    );
    std::time_t t = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

/// 提取文件扩展名（不含 '.'），如 "txt"、"bin"，无扩展名返回空
std::string GetFileExtension(const std::string& name) {
    auto pos = name.rfind('.');
    if (pos == std::string::npos || pos == name.size() - 1) return "";
    // 确保不是目录分隔符后面的点（如 ".hidden" 算无扩展名）
    auto slash_pos = name.rfind('/');
    if (slash_pos != std::string::npos && pos < slash_pos) return "";
    // 检查文件名部分是否以点开头且没有其他点
    std::string basename = (slash_pos != std::string::npos) ? name.substr(slash_pos + 1) : name;
    auto dot_pos = basename.rfind('.');
    if (dot_pos == 0 && basename.find('.', 1) == std::string::npos) return "";
    return name.substr(pos + 1);
}

}  // namespace

// ============================================================================
// 状态码/错误码转字符串
// ============================================================================

const char* CacheStatusToString(CacheStatus status) {
    switch (status) {
        case CacheStatus::NotInitialized: return "NotInitialized";
        case CacheStatus::Running:        return "Running";
        case CacheStatus::Error:          return "Error";
        default:                          return "Unknown";
    }
}

const char* CacheErrorCodeToString(CacheErrorCode code) {
    switch (code) {
        case CacheErrorCode::Ok:               return "Ok";
        case CacheErrorCode::NotInitialized:   return "NotInitialized";
        case CacheErrorCode::PathTraversal:    return "PathTraversal";
        case CacheErrorCode::FileNotFound:     return "FileNotFound";
        case CacheErrorCode::FileAlreadyExists:return "FileAlreadyExists";
        case CacheErrorCode::IoError:          return "IoError";
        case CacheErrorCode::InvalidArgument:  return "InvalidArgument";
        case CacheErrorCode::CreateDirFailed:  return "CreateDirFailed";
        case CacheErrorCode::FileTooLarge:     return "FileTooLarge";
        default:                               return "Unknown";
    }
}

// ============================================================================
// MyCache 实现
// ============================================================================

MyCache::MyCache() {
    MYLOG_DEBUG("[MyCache] 默认构造，等待 Init() 调用");
}

MyCache::~MyCache() {
    MYLOG_INFO("[MyCache] 开始析构，停止后台扫描线程...");

    // 通知后台线程退出
    running_.store(false);
    cv_.notify_all();

    // 等待线程结束
    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }

    MYLOG_INFO("[MyCache] 析构完成");
}

// ============================================================================
// Init
// ============================================================================

CacheResult<void> MyCache::Init(const std::string& config_json) {
    MYLOG_INFO("[MyCache] Init 被调用，配置 JSON：{}", config_json);

    // 防止重复初始化
    if (status_.load() == CacheStatus::Running) {
        MYLOG_WARN("[MyCache] 已经初始化过，忽略重复调用");
        return CacheResult<void>::Success();
    }

    // 1. 解析 JSON 配置
    json jcfg;
    try {
        jcfg = json::parse(config_json);
    } catch (const json::parse_error& e) {
        MYLOG_ERROR("[MyCache] JSON 解析失败：{}", e.what());
        status_.store(CacheStatus::Error);
        return CacheResult<void>::Fail(CacheErrorCode::InvalidArgument);
    }
    
    // 2. 提取配置项
    if (!jcfg.contains("root_path") || !jcfg["root_path"].is_string()) {
        MYLOG_ERROR("[MyCache] JSON 缺少必填字段 root_path 或类型错误");
        status_.store(CacheStatus::Error);
        return CacheResult<void>::Fail(CacheErrorCode::InvalidArgument);
    }
    config_.root_path = jcfg["root_path"].get<std::string>();

    if (jcfg.contains("max_file_size") && jcfg["max_file_size"].is_number_unsigned()) {
        config_.max_file_size = jcfg["max_file_size"].get<uint64_t>();
    }
    if (jcfg.contains("max_retention_seconds") && jcfg["max_retention_seconds"].is_number()) {
        config_.max_retention_seconds = jcfg["max_retention_seconds"].get<int64_t>();
    }

    MYLOG_INFO("[MyCache] 配置解析完成：root_path={}, max_file_size={}, max_retention_seconds={}",
               config_.root_path, config_.max_file_size, config_.max_retention_seconds);

    // 3. 对传入路径进行规范化处理
    std::error_code ec;
    std::filesystem::path p = std::filesystem::absolute(config_.root_path, ec);
    if (ec) {
        MYLOG_ERROR("[MyCache] 无法获取绝对路径：{}, 错误：{}", config_.root_path, ec.message());
        status_.store(CacheStatus::Error);
        return CacheResult<void>::Fail(CacheErrorCode::InvalidArgument);
    }

    // 如果目录不存在，尝试创建
    if (!std::filesystem::exists(p, ec)) {
        MYLOG_WARN("[MyCache] 目录不存在，尝试创建：{}", p.string());
        if (!std::filesystem::create_directories(p, ec) || ec) {
            MYLOG_ERROR("[MyCache] 创建目录失败：{}, 错误：{}", p.string(), ec.message());
            status_.store(CacheStatus::Error);
            return CacheResult<void>::Fail(CacheErrorCode::CreateDirFailed);
        }
        MYLOG_INFO("[MyCache] 目录创建成功：{}", p.string());
    }

    // 确保路径是目录
    if (!std::filesystem::is_directory(p, ec)) {
        MYLOG_ERROR("[MyCache] 路径不是目录：{}", p.string());
        status_.store(CacheStatus::Error);
        return CacheResult<void>::Fail(CacheErrorCode::InvalidArgument);
    }

    // 获取规范化的绝对路径（解析符号链接等）
    root_path_ = std::filesystem::canonical(p, ec);
    if (ec) {
        MYLOG_ERROR("[MyCache] 规范化路径失败：{}, 错误：{}", p.string(), ec.message());
        status_.store(CacheStatus::Error);
        return CacheResult<void>::Fail(CacheErrorCode::InvalidArgument);
    }

    MYLOG_INFO("[MyCache] 规范化根目录：{}", root_path_.string());

    // 4. 首次扫描建立索引
    RefreshIndex();

    // 5. 启动后台扫描线程
    status_.store(CacheStatus::Running);
    running_.store(true);
    scan_thread_ = std::thread(&MyCache::ScanThreadFunc, this);

    MYLOG_INFO("[MyCache] 初始化完成，后台扫描线程已启动，扫描间隔 {} 秒", kScanIntervalSec);
    return CacheResult<void>::Success();
}

CacheStatus MyCache::Status() const {
    return status_.load();
}

std::string MyCache::GetRootPath() const {
    return root_path_.string();
}

const CacheConfig& MyCache::GetConfig() const {
    return config_;
}

// ============================================================================
// 路径穿越校验
// ============================================================================

CacheErrorCode MyCache::ValidatePath(const std::string& name,
                                     std::filesystem::path& full_path) const {
    // 检查文件名是否为空
    if (name.empty()) {
        MYLOG_WARN("[MyCache] 文件名为空");
        return CacheErrorCode::InvalidArgument;
    }

    // 检查模块是否已初始化
    if (status_.load() != CacheStatus::Running) {
        MYLOG_WARN("[MyCache] 模块尚未初始化");
        return CacheErrorCode::NotInitialized;
    }

    // 拼接路径并进行弱规范化（不要求路径实际存在）
    std::filesystem::path candidate = root_path_ / name;
    // weakly_canonical: 对已存在的部分做 canonical，不存在的部分保持原样
    std::error_code ec;
    std::filesystem::path resolved = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) {
        MYLOG_WARN("[MyCache] 路径解析失败：{}, 错误：{}", candidate.string(), ec.message());
        return CacheErrorCode::InvalidArgument;
    }

    // 核心安全检查：确保解析后的路径以 root_path_ 为前缀
    // 使用字符串前缀匹配，确保不会穿越到 root_path_ 之外
    std::string root_str = root_path_.string() + "/";
    std::string resolved_str = resolved.string();

    // 如果 resolved 恰好等于 root_path_ 本身（用户传了 "." 或 ""），也应拒绝
    if (resolved_str != root_path_.string() &&
        resolved_str.substr(0, root_str.size()) != root_str) {
        MYLOG_ERROR("[MyCache] 路径穿越攻击被阻止！请求路径：{}, 解析后：{}, 根目录：{}",
                    name, resolved_str, root_path_.string());
        return CacheErrorCode::PathTraversal;
    }

    // 额外检查：resolved 不应恰好等于 root_path_（用户不应操作根目录本身）
    if (resolved_str == root_path_.string()) {
        MYLOG_WARN("[MyCache] 不允许操作根目录本身：{}", name);
        return CacheErrorCode::InvalidArgument;
    }

    full_path = resolved;
    return CacheErrorCode::Ok;
}

// ============================================================================
// 文件操作接口
// ============================================================================

CacheResult<void> MyCache::SaveFile(const std::string& name,
                                    const std::vector<uint8_t>& data) {
    MYLOG_DEBUG("[MyCache] SaveFile 请求：name={}, 数据大小={} 字节", name, data.size());

    std::filesystem::path full_path;
    auto code = ValidatePath(name, full_path);
    if (code != CacheErrorCode::Ok) {
        MYLOG_WARN("[MyCache] SaveFile 路径校验失败：name={}, 错误码={}", name, CacheErrorCodeToString(code));
        return CacheResult<void>::Fail(code);
    }

    // 检查文件大小限制
    if (config_.max_file_size > 0 && data.size() > config_.max_file_size) {
        MYLOG_WARN("[MyCache] 文件超过大小限制：name={}, 大小={} 字节, 限制={} 字节",
                   name, data.size(), config_.max_file_size);
        return CacheResult<void>::Fail(CacheErrorCode::FileTooLarge);
    }

    // 确保父目录存在
    std::error_code ec;
    auto parent = full_path.parent_path();
    if (!std::filesystem::exists(parent, ec)) {
        MYLOG_DEBUG("[MyCache] 创建父目录：{}", parent.string());
        if (!std::filesystem::create_directories(parent, ec) || ec) {
            MYLOG_ERROR("[MyCache] 创建父目录失败：{}, 错误：{}", parent.string(), ec.message());
            return CacheResult<void>::Fail(CacheErrorCode::CreateDirFailed);
        }
    }

    // 写入文件（原子：先写临时文件再重命名）
    auto tmp_path = full_path;
    tmp_path += ".tmp";

    {
        std::ofstream ofs(tmp_path, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            MYLOG_ERROR("[MyCache] 无法打开临时文件：{}", tmp_path.string());
            return CacheResult<void>::Fail(CacheErrorCode::IoError);
        }
        ofs.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
        if (!ofs.good()) {
            MYLOG_ERROR("[MyCache] 写入临时文件失败：{}", tmp_path.string());
            ofs.close();
            std::filesystem::remove(tmp_path, ec);
            return CacheResult<void>::Fail(CacheErrorCode::IoError);
        }
    }

    // 重命名临时文件为目标文件
    std::filesystem::rename(tmp_path, full_path, ec);
    if (ec) {
        MYLOG_ERROR("[MyCache] 重命名文件失败：{} -> {}, 错误：{}",
                    tmp_path.string(), full_path.string(), ec.message());
        std::filesystem::remove(tmp_path, ec);
        return CacheResult<void>::Fail(CacheErrorCode::IoError);
    }

    // 立即更新内存索引
    {
        FileInfo info;
        info.name = name;
        info.size = data.size();
        info.type = GetFileExtension(name);
        info.modified_at = FormatFileTime(std::filesystem::last_write_time(full_path, ec));

        std::unique_lock lock(index_mutex_);
        file_index_[name] = std::move(info);
    }

    MYLOG_INFO("[MyCache] 文件保存成功：{}, 大小={} 字节", name, data.size());
    return CacheResult<void>::Success();
}

CacheResult<void> MyCache::DeleteFile(const std::string& name) {
    MYLOG_DEBUG("[MyCache] DeleteFile 请求：name={}", name);

    std::filesystem::path full_path;
    auto code = ValidatePath(name, full_path);
    if (code != CacheErrorCode::Ok) {
        MYLOG_WARN("[MyCache] DeleteFile 路径校验失败：name={}, 错误码={}", name, CacheErrorCodeToString(code));
        return CacheResult<void>::Fail(code);
    }

    std::error_code ec;
    if (!std::filesystem::exists(full_path, ec)) {
        MYLOG_WARN("[MyCache] 删除目标不存在：{}", full_path.string());
        return CacheResult<void>::Fail(CacheErrorCode::FileNotFound);
    }

    if (!std::filesystem::remove(full_path, ec) || ec) {
        MYLOG_ERROR("[MyCache] 删除文件失败：{}, 错误：{}", full_path.string(), ec.message());
        return CacheResult<void>::Fail(CacheErrorCode::IoError);
    }

    // 立即更新内存索引
    {
        std::unique_lock lock(index_mutex_);
        file_index_.erase(name);
    }

    MYLOG_INFO("[MyCache] 文件删除成功：{}", name);
    return CacheResult<void>::Success();
}

CacheResult<std::string> MyCache::GetFullPath(const std::string& name) {
    MYLOG_DEBUG("[MyCache] GetFullPath 请求：name={}", name);

    std::filesystem::path full_path;
    auto code = ValidatePath(name, full_path);
    if (code != CacheErrorCode::Ok) {
        MYLOG_WARN("[MyCache] GetFullPath 路径校验失败：name={}, 错误码={}", name, CacheErrorCodeToString(code));
        return CacheResult<std::string>::Fail(code);
    }

    MYLOG_DEBUG("[MyCache] GetFullPath 结果：{} -> {}", name, full_path.string());
    return CacheResult<std::string>::Success(full_path.string());
}

CacheResult<bool> MyCache::Exists(const std::string& name) {
    // Exists 是高频调用，仅在 debug 级别记录
    MYLOG_DEBUG("[MyCache] Exists 查询：name={}", name);

    std::filesystem::path full_path;
    auto code = ValidatePath(name, full_path);
    if (code != CacheErrorCode::Ok) {
        return CacheResult<bool>::Fail(code, false);
    }

    // O(1) 查询内存索引（使用读锁）
    {
        std::shared_lock lock(index_mutex_);
        bool found = file_index_.count(name) > 0;
        MYLOG_DEBUG("[MyCache] Exists 结果：{} -> {}", name, found);
        return CacheResult<bool>::Success(found);
    }
}

CacheResult<std::vector<FileInfo>> MyCache::GetAllFileList() {
    MYLOG_DEBUG("[MyCache] GetAllFileList 请求");

    if (status_.load() != CacheStatus::Running) {
        MYLOG_WARN("[MyCache] GetAllFileList 调用时模块未初始化");
        return CacheResult<std::vector<FileInfo>>::Fail(CacheErrorCode::NotInitialized);
    }

    std::vector<FileInfo> result;
    {
        std::shared_lock lock(index_mutex_);
        result.reserve(file_index_.size());
        for (const auto& [key, info] : file_index_) {
            result.push_back(info);
        }
    }

    MYLOG_DEBUG("[MyCache] GetAllFileList 返回 {} 个文件", result.size());
    return CacheResult<std::vector<FileInfo>>::Success(std::move(result));
}

// ============================================================================
// 后台扫描线程
// ============================================================================

void MyCache::ScanThreadFunc() {
    MYLOG_INFO("[MyCache] 后台扫描线程启动");

    while (running_.load()) {
        // 等待指定时间或被唤醒退出
        {
            std::unique_lock lock(cv_mutex_);
            cv_.wait_for(lock, std::chrono::seconds(kScanIntervalSec), [this]() {
                return !running_.load();
            });
        }

        if (!running_.load()) {
            break;
        }

        // 执行一次目录扫描
        RefreshIndex();

        // 清理过期文件
        CleanExpiredFiles();
    }

    MYLOG_INFO("[MyCache] 后台扫描线程退出");
}

void MyCache::RefreshIndex() {
    MYLOG_DEBUG("[MyCache] 开始扫描目录：{}", root_path_.string());

    std::unordered_map<std::string, FileInfo> new_index;
    std::error_code ec;

    // 递归遍历缓存目录下的所有常规文件
    for (auto it = std::filesystem::recursive_directory_iterator(root_path_, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            MYLOG_WARN("[MyCache] 遍历过程中出错：{}", ec.message());
            ec.clear();
            continue;
        }

        if (it->is_regular_file(ec) && !ec) {
            // 计算相对于 root_path_ 的相对路径作为索引 key
            auto rel = std::filesystem::relative(it->path(), root_path_, ec);
            if (ec) continue;

            std::string rel_str = rel.string();
            FileInfo info;
            info.name = rel_str;
            info.size = it->file_size(ec);
            if (ec) { info.size = 0; ec.clear(); }
            info.type = GetFileExtension(rel_str);

            auto ftime = it->last_write_time(ec);
            if (!ec) {
                info.modified_at = FormatFileTime(ftime);
            } else {
                info.modified_at = "";
                ec.clear();
            }

            new_index[rel_str] = std::move(info);
        }
    }

    // 用写锁替换整个索引
    {
        std::unique_lock lock(index_mutex_);
        file_index_.swap(new_index);
    }

    MYLOG_DEBUG("[MyCache] 扫描完成，索引文件数量：{}", file_index_.size());
}

void MyCache::CleanExpiredFiles() {
    if (config_.max_retention_seconds <= 0) {
        return;  // 未配置过期策略
    }

    MYLOG_DEBUG("[MyCache] 开始清理过期文件（保留时间 {} 秒）", config_.max_retention_seconds);

    auto now = std::chrono::system_clock::now();
    std::vector<std::string> expired_names;

    // 在读锁下收集过期文件（不在锁内做删除操作）
    {
        std::shared_lock lock(index_mutex_);
        for (const auto& [name, info] : file_index_) {
            std::error_code ec;
            auto full_path = root_path_ / name;
            auto ftime = std::filesystem::last_write_time(full_path, ec);
            if (ec) continue;

            // 将 file_time_type 转为 system_clock
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now()
                     + std::chrono::system_clock::now()
            );

            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - sctp).count();
            if (age > config_.max_retention_seconds) {
                expired_names.push_back(name);
            }
        }
    }

    // 删除过期文件
    for (const auto& name : expired_names) {
        std::error_code ec;
        auto full_path = root_path_ / name;
        if (std::filesystem::remove(full_path, ec) && !ec) {
            MYLOG_INFO("[MyCache] 过期文件已清理：{}", name);
            std::unique_lock lock(index_mutex_);
            file_index_.erase(name);
        } else {
            MYLOG_WARN("[MyCache] 清理过期文件失败：{}, 错误：{}", name, ec.message());
        }
    }

    if (!expired_names.empty()) {
        MYLOG_INFO("[MyCache] 过期清理完成，共清理 {} 个文件", expired_names.size());
    }
}

}  // namespace my_cache
