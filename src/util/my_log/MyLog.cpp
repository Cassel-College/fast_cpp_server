#include "MyLog.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "spdlog/sinks/stdout_color_sinks.h"

namespace MyLog {

static std::shared_ptr<spdlog::logger> logger;

namespace fs = std::filesystem;

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

void ArchiveOldLogs(const std::string& log_dir, const std::string& archive_dir, std::vector<std::string>& logInfos) {
    std::cout << "检查并归档旧日志文件..." << std::endl;
    std::cout << "日志目录: " << log_dir << std::endl;
    std::cout << "归档目录: " << archive_dir << std::endl;
    try {
        if (!fs::exists(log_dir) || !fs::is_directory(log_dir)) {
            logInfos.push_back("日志文件夹不存在: " + log_dir);
            return;  // 没有日志目录，直接返回
        }

        // 检测是否存在日志文件
        bool has_log = false;
        for (const auto& entry : fs::directory_iterator(log_dir)) {
            if (entry.is_regular_file()) {
                has_log = true;
                break;
            }
        }

        if (!has_log) {
            logInfos.push_back("没有残留日志文件需要归档.");
            return;  // 没有残留日志
        } else {
            logInfos.push_back("发现残留日志文件，开始归档...");
        }

        // 创建 archive/时间戳 目录
        fs::path archive_path = fs::path(log_dir) / archive_dir / CurrentTimestamp();
        fs::create_directories(archive_path);
        logInfos.push_back("归档目录创建于: " + archive_path.string());
        // 移动所有日志文件（排除 archive 自身）
        for (const auto& entry : fs::directory_iterator(log_dir)) {
            if (!entry.is_regular_file()) continue;

            fs::path src = entry.path();
            fs::path dst = archive_path / src.filename();

            fs::rename(src, dst);
            logInfos.push_back("归档日志文件: " + src.string() + " -> " + dst.string());
        }
    }
    catch (const std::exception& e) {
        // 这里不能用 spdlog（还没初始化）
        std::cerr << "[LogArchive] Failed: " << e.what() << std::endl;
    }
}


void Init(const std::string& log_file, size_t max_file_size, size_t max_files, bool console_output) {

    std::cout << "初始化日志系统..." << std::endl;
    std::cout << "日志文件: " << log_file << std::endl;
    
    std::vector<std::string> logInfos = {};
    std::string archive_dir = "archive";
    // ① 从 log_file 推导日志目录
    fs::path log_path(log_file);
    fs::path log_dir = log_path.parent_path();

    // 如果没有目录（例如 "server.log"）
    if (log_dir.empty()) {
        log_dir = ".";  // 当前目录
    }
    
    if (!fs::exists(log_dir)) {
        logInfos.push_back("日志目录不存在，创建目录: " + log_dir.string());
        std::cout << "日志目录不存在，创建目录: " << log_dir.string() << std::endl;
        try {
            fs::create_directories(log_dir);
            logInfos.push_back("日志目录创建成功: " + log_dir.string());
        } catch (const std::exception& e) {
            logInfos.push_back(
                std::string("[MyLog] 创建日志目录失败: ") + e.what()
            );
            std::cerr << "[MyLog] 创建日志目录失败: " << e.what() << std::endl;
        }
    } else {
        logInfos.push_back("日志目录存在: " + log_dir.string());
    }
    std::cout << "日志目录: " << log_dir.string() << std::endl;
    // ② 启动前归档
    ArchiveOldLogs(log_dir.string(), archive_dir, logInfos);
    
    // 异步线程池初始化
    spdlog::init_thread_pool(8192, 1);

    // 1. 创建文件输出 Sink
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, max_file_size, max_files);

    // 2. 创建终端输出 Sink (带颜色)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug); // 可以单独设置终端显示的级别

    // 3. 组合两个 Sinks
    std::vector<spdlog::sink_ptr> sinks = {};
    // {file_sink, console_sink};
    if (true) {
        sinks.push_back(file_sink);
    }
    if (console_output) {
        sinks.push_back(console_sink);
    }
    

    // 4. 创建异步 logger，传入 sinks 列表
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
        MYLOG_INFO(logItem);
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
