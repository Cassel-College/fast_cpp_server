
#include "PodConfig.h"
#include "MyLog.h"

#include <cerrno>
#include <sstream>
#include <unistd.h>

namespace pod_stream {
// ============================================================================
// PodConfig 实现
// ============================================================================

bool PodConfig::IsValid() const {
    std::string tempLog = "";
    if (!open) {
        return true;  // 功能关闭视为有效配置
    }
    
    if (pod_ip.empty()) {
        tempLog = std::string("配置错误: pod_ip 为空");
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    if (abs_mediamtx_path.empty()) {
        tempLog = std::string("配置错误: abs_mediamtx_path 为空");
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    // 检查 MediaMTX 可执行文件是否存在且可执行
    if (access(abs_mediamtx_path.c_str(), X_OK) != 0) {
        tempLog = std::string("配置错误: MediaMTX 不存在或不可执行: " + 
              abs_mediamtx_path + " (errno=" + std::to_string(errno) + ")");
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    if (stream_mapping.empty()) {
        tempLog = std::string("配置错误: stream_mapping 为空");
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    if (rtsp_port.empty()) {
        tempLog = std::string("配置错误: rtsp_port 为空");
        MYLOG_INFO(tempLog.c_str());
        return false;
    }
    
    return true;
}

void PodConfig::Print() const {
    std::string tempLog = "";
    std::ostringstream oss;
    oss << "PodConfig {"
        << "\n  open: " << (open ? "true" : "false")
        << "\n  pod_ip: " << pod_ip
        << "\n  rtsp_port: " << rtsp_port
        << "\n  abs_mediamtx_path: " << abs_mediamtx_path
        << "\n  stream_mapping: [";
    
    for (const auto& kv : stream_mapping) {
        oss << "\n    " << kv.first << " -> " << kv.second;
    }
    oss << "\n  ]"
        << "\n  monitor_period_ms: " << monitor_period_ms
        << "\n  worker_period_ms: " << worker_period_ms
        << "\n  connect_debounce_count: " << connect_debounce_count
        << "\n  disconnect_debounce_count: " << disconnect_debounce_count
        << "\n}";
    
    tempLog = std::string(oss.str());
    MYLOG_INFO(tempLog.c_str());
}



/**
 * @brief 创建默认/示例配置
 * @return 默认配置
 */
PodConfig CreateDefaultConfig() {
    PodConfig config;
    // 基本配置
    config.open                         = true;
    config.pod_ip                       = "192.168.10.251";
    config.rtsp_port                    = "8555";
    config.abs_mediamtx_path            = "/root/test_rtsp/mediamtx";
    config.stream_mapping["video_r"]    = "rtsp://192.168.10.251:8554/video_r";    // 流映射: 输出路径 -> 输入 URL
    config.stream_mapping["video_l"]    = "rtsp://192.168.10.251:8554/video_l";
    config.stream_mapping["video_m"]    = "rtsp://192.168.10.251:8554/video_m";
    config.monitor_period_ms            = 500;                                     // 调优参数 (使用默认值)                             
    config.worker_period_ms             = 200;
    config.connect_debounce_count       = 3;
    config.disconnect_debounce_count    = 3;
    config.graceful_stop_timeout_ms     = 3000;
    config.max_crash_count              = -1;                                       // -1 表示不限制
    config.crash_window_seconds         = 60;
    config.Print();
    return config;
}


PodConfig CreatePodConfigByUAV() {
    MYLOG_INFO("开始读取UAV配置文件构造Pod Stream Manager初始化参数");
    PodConfig config;
    
    return config;
}

PodConfig CreatePodByConfig(const nlohmann::json& json_config) {
    PodConfig config;

    config.open                 = json_config.value("mediamtx_enable", false);
    config.pod_ip               = json_config.value("check_ip", std::string());
    config.abs_mediamtx_path    = json_config.value("mediamtx_bin_path", std::string());

    if (json_config.contains("mediamtx_json_config") && json_config["mediamtx_json_config"].is_object()) {
        const auto& mediamtx_json_config = json_config["mediamtx_json_config"];

        config.rtsp_port = mediamtx_json_config.value("rtspAddress", config.rtsp_port);
        if (!config.rtsp_port.empty() && config.rtsp_port.front() == ':') {
            config.rtsp_port.erase(0, 1);
        }

        if (mediamtx_json_config.contains("paths") && mediamtx_json_config["paths"].is_object()) {
            for (const auto& [path_name, path_config] : mediamtx_json_config["paths"].items()) {
                if (!path_config.is_object()) {
                    continue;
                }
                if (!path_config.contains("source") || !path_config["source"].is_string()) {
                    continue;
                }
                config.stream_mapping[path_name] = path_config["source"].get<std::string>();
            }
        }
    }

    // config.monitor_period_ms = json_config.value("monitor_period_ms", config.monitor_period_ms);
    // config.worker_period_ms = json_config.value("worker_period_ms", config.worker_period_ms);
    // config.connect_debounce_count = json_config.value("connect_debounce_count", config.connect_debounce_count);
    // config.disconnect_debounce_count = json_config.value("disconnect_debounce_count", config.disconnect_debounce_count);
    // config.graceful_stop_timeout_ms = json_config.value("graceful_stop_timeout_ms", config.graceful_stop_timeout_ms);
    // config.max_crash_count = json_config.value("max_crash_count", config.max_crash_count);
    // config.crash_window_seconds = json_config.value("crash_window_seconds", config.crash_window_seconds);

    config.monitor_period_ms            = 500;                                     // 调优参数 (使用默认值)                             
    config.worker_period_ms             = 200;
    config.connect_debounce_count       = 3;
    config.disconnect_debounce_count    = 3;
    config.graceful_stop_timeout_ms     = 3000;
    config.max_crash_count              = -1;                                       // -1 表示不限制
    config.crash_window_seconds         = 60;
    

    config.Print();
    return config;
}

} // namespace pod_stream