#ifndef POD_CONFIG_HPP
#define POD_CONFIG_HPP

#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <sys/types.h>
#include <nlohmann/json.hpp>

// ============================================================================
// 配置结构体 (Configuration Structures)
// ============================================================================
namespace pod_stream {
/**
 * @brief 吊舱配置结构体
 * 
 * 包含吊舱连接所需的所有配置参数，支持多路流映射。
 */
struct PodConfig {
    bool open{false};                              ///< 是否开启吊舱流媒体功能
    std::string pod_ip;                            ///< 吊舱 IP 地址
    std::string rtsp_port{"8555"};                 ///< RTSP 服务端口
    std::string abs_mediamtx_path;                 ///< MediaMTX 可执行文件绝对路径
    
    /**
     * @brief 流地址映射表
     * 
     * key: 输出流路径名 (如 "video_r")
     * value: 输入流 RTSP URL (如 "rtsp://192.168.10.251:8554/video_r")
     */
    std::map<std::string, std::string> stream_mapping;
    
    // ========== 可调参数 (Tunable Parameters) ==========
    int monitor_period_ms{500};       ///< 连通性探测周期 (毫秒)
    int worker_period_ms{200};        ///< 状态机调度周期 (毫秒)
    int connect_debounce_count{3};    ///< 连续成功 N 次才认为 connected
    int disconnect_debounce_count{3}; ///< 连续失败 N 次才认为 disconnected
    int graceful_stop_timeout_ms{3000}; ///< SIGTERM 后等待退出的超时时间 (毫秒)
    int max_crash_count{-1};           ///< 短时间内最大崩溃次数阈值 -1 表示不限制
    int crash_window_seconds{60};     ///< 崩溃计数时间窗口 (秒)
    
    /**
     * @brief 验证配置有效性
     * @return 配置是否有效
     */
    bool IsValid() const;
    
    /**
     * @brief 打印配置信息 (调试用)
     */
    void Print() const;
};

/**
 * @brief 创建默认/示例配置
 * @return 默认配置
 */
PodConfig CreateDefaultConfig();

/**
 * @brief 创建无人机制定的配置
 * @return 配置文件的配置
 */
PodConfig CreatePodConfigByUAV();


PodConfig CreatePodByConfig(const nlohmann::json& json_config);


} // namespace pod_stream


#endif // POD_CONFIG_HPP
