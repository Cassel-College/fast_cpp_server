#include "MyLog.h"
#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <iostream>
#include <vector>
#include "spdlog/sinks/stdout_color_sinks.h"

namespace MyLog {

static std::shared_ptr<spdlog::logger> logger;

namespace {

std::string NormalizePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string JoinPath(const std::string& base, const std::string& name) {
    if (base.empty() || base == ".") {
        return name;
    }
    if (name.empty()) {
        return base;
    }
    if (base.back() == '/') {
        return base + name;
    }
    return base + "/" + name;
}

std::string GetParentDirectory(const std::string& path) {
    const std::string normalized = NormalizePath(path);
    if (normalized.empty()) {
        return ".";
    }

    const std::size_t end = normalized.find_last_not_of('/');
    if (end == std::string::npos) {
        return "/";
    }

    const std::size_t pos = normalized.find_last_of('/', end);
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "/";
    }
    return normalized.substr(0, pos);
}

std::string GetFilename(const std::string& path) {
    const std::string normalized = NormalizePath(path);
    const std::size_t pos = normalized.find_last_of('/');
    if (pos == std::string::npos) {
        return normalized;
    }
    return normalized.substr(pos + 1);
}

bool GetPathStat(const std::string& path, struct stat& status) {
    return ::stat(path.c_str(), &status) == 0;
}

bool DirectoryExists(const std::string& path) {
    struct stat status {};
    return GetPathStat(path, status) && S_ISDIR(status.st_mode);
}

bool IsRegularFile(const std::string& path) {
    struct stat status {};
    return GetPathStat(path, status) && S_ISREG(status.st_mode);
}

bool EnsureDirectoryExists(const std::string& path, std::string& error_message) {
    const std::string normalized = NormalizePath(path);
    if (normalized.empty() || normalized == ".") {
        return true;
    }

    if (DirectoryExists(normalized)) {
        return true;
    }

    std::string current;
    std::size_t index = 0;
    if (!normalized.empty() && normalized.front() == '/') {
        current = "/";
        index = 1;
    }

    while (index <= normalized.size()) {
        const std::size_t next = normalized.find('/', index);
        const std::string part = normalized.substr(index, next == std::string::npos ? std::string::npos : next - index);
        if (!part.empty() && part != ".") {
            if (!current.empty() && current.back() != '/') {
                current += '/';
            }
            current += part;

            if (!DirectoryExists(current)) {
                if (::mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
                    error_message = std::string("[MyLog] 创建日志目录失败: ") + current + ", error=" + std::strerror(errno);
                    return false;
                }
            }
        }

        if (next == std::string::npos) {
            break;
        }
        index = next + 1;
    }

    return true;
}

static std::string CurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void ArchiveOldLogsInternal(const std::string& log_dir,
                            const std::string& archive_dir,
                            std::vector<std::string>& logInfos) {
    std::cout << "检查并归档旧日志文件..." << std::endl;
    std::cout << "日志目录: " << log_dir << std::endl;
    std::cout << "归档目录: " << archive_dir << std::endl;
    if (!DirectoryExists(log_dir)) {
        logInfos.push_back("日志文件夹不存在: " + log_dir);
        return;
    }

    std::vector<std::string> files_to_archive;
    DIR* dir = ::opendir(log_dir.c_str());
    if (dir == nullptr) {
        logInfos.push_back(std::string("[LogArchive] 打开日志目录失败: ") + std::strerror(errno));
        return;
    }

    while (dirent* entry = ::readdir(dir)) {
        const std::string file_name = entry->d_name;
        if (file_name == "." || file_name == "..") {
            continue;
        }

        const std::string full_path = JoinPath(log_dir, file_name);
        if (IsRegularFile(full_path)) {
            files_to_archive.push_back(full_path);
        }
    }
    ::closedir(dir);

    if (files_to_archive.empty()) {
        logInfos.push_back("没有残留日志文件需要归档.");
        return;
    }

    logInfos.push_back("发现残留日志文件，开始归档...");

    const std::string archive_root = JoinPath(log_dir, archive_dir);
    const std::string archive_path = JoinPath(archive_root, CurrentTimestamp());
    std::string mkdir_error;
    if (!EnsureDirectoryExists(archive_path, mkdir_error)) {
        logInfos.push_back(mkdir_error);
        std::cerr << mkdir_error << std::endl;
        return;
    }

    logInfos.push_back("归档目录创建于: " + archive_path);
    for (const std::string& src : files_to_archive) {
        const std::string dst = JoinPath(archive_path, GetFilename(src));
        if (::rename(src.c_str(), dst.c_str()) == 0) {
            logInfos.push_back("归档日志文件: " + src + " -> " + dst);
        } else {
            const std::string error = std::string("[LogArchive] 归档失败: ") + src + " -> " + dst + ", error=" + std::strerror(errno);
            logInfos.push_back(error);
            std::cerr << error << std::endl;
        }
    }
}

}  // namespace

void ArchiveOldLogs(const std::string& log_dir, const std::string& archive_dir) {
    std::vector<std::string> ignored_logs;
    ArchiveOldLogsInternal(log_dir, archive_dir, ignored_logs);
}


void Init(const std::string& log_file, size_t max_file_size, size_t max_files, bool console_output) {

    std::cout << "初始化日志系统..." << std::endl;
    std::cout << "日志文件: " << log_file << std::endl;
    
    std::vector<std::string> logInfos = {};
    std::string archive_dir = "archive";
    const std::string log_dir = GetParentDirectory(log_file);
    
    if (!DirectoryExists(log_dir)) {
        logInfos.push_back("日志目录不存在，创建目录: " + log_dir);
        std::cout << "日志目录不存在，创建目录: " << log_dir << std::endl;
        std::string mkdir_error;
        if (EnsureDirectoryExists(log_dir, mkdir_error)) {
            logInfos.push_back("日志目录创建成功: " + log_dir);
        } else {
            logInfos.push_back(mkdir_error);
            std::cerr << mkdir_error << std::endl;
        }
    } else {
        logInfos.push_back("日志目录存在: " + log_dir);
    }
    std::cout << "日志目录: " << log_dir << std::endl;
    ArchiveOldLogsInternal(log_dir, archive_dir, logInfos);
    
    spdlog::init_thread_pool(8192, 1);

    std::vector<spdlog::sink_ptr> sinks;
    bool file_sink_ready = false;
    try {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, max_file_size, max_files);
        sinks.push_back(file_sink);
        file_sink_ready = true;
    } catch (const std::exception& e) {
        logInfos.push_back(std::string("[MyLog] 文件日志初始化失败，已回退到控制台: ") + e.what());
        std::cerr << "[MyLog] 文件日志初始化失败，已回退到控制台: " << e.what() << std::endl;
    }

    if (console_output || !file_sink_ready) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        sinks.push_back(console_sink);
    }

    if (sinks.empty()) {
        throw std::runtime_error("[MyLog] 没有可用的日志输出通道。");
    }

    logger = std::make_shared<spdlog::async_logger>(
        "async_logger", sinks.begin(), sinks.end(),
        spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    // 设置为默认 logger
    spdlog::set_default_logger(logger);

    // 设置全局格式
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%# %!] %v");
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::info);

    for(const std::string& logItem : logInfos) {
        MYLOG_INFO("{}", logItem);
    }
}

void Info(const std::string& msg) {
    spdlog::info(msg);
}

void Debug(const std::string& msg) {
    spdlog::debug(msg);
}

void Warn(const std::string& msg) {
    spdlog::warn(msg);
}

void Error(const std::string& msg) {
    spdlog::error(msg);
}

void Flush() {
    if (logger) {
        logger->flush();
    }
}

}  // namespace MyLog
