#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <memory>
#include <string>

namespace MyLog {

// 初始化日志系统（异步、文件输出、最大大小与滚动数）
void Init(const std::string& log_file = "logs/server.log",
          size_t max_file_size = 1048576 * 5,  // 5MB
          size_t max_files = 3,                // 最多保留3个文件
          bool console_output = false);        // 是否输出到控制台

// 可选封装函数
void Info(const std::string& msg);
void Debug(const std::string& msg);
void Warn(const std::string& msg);
void Error(const std::string& msg);
void Flush();

/**
 * @brief 启动时归档上一次运行残留的日志文件
 * @param log_dir 日志目录，如 "logs"
 * @param archive_dir 归档目录名，默认 "archive"
 */
void ArchiveOldLogs(const std::string& log_dir, const std::string& archive_dir = "archive");

// 推荐使用的宏（可输出文件、行号、函数名）
#define MYLOG_INFO(...) \
    spdlog::log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::info, __VA_ARGS__)

#define MYLOG_DEBUG(...) \
    spdlog::log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::debug, __VA_ARGS__)

#define MYLOG_WARN(...) \
    spdlog::log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::warn, __VA_ARGS__)

#define MYLOG_ERROR(...) \
    spdlog::log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::err, __VA_ARGS__)

}  // namespace MyLog
