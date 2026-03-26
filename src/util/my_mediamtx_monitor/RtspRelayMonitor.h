#pragma once

/**
 * @file RtspRelayMonitor.h
 * @brief RTSP 转发守护模块
 *
 * 模块职责：
 *   1. 解析 JSON 配置（enable + MediaMTX 配置）
 *   2. 将 JSON 配置转换为 MediaMTX 所需的 YAML 配置文件
 *   3. 周期性检测 RTSP 源连通性（TCP socket 探测）
 *   4. 管理 MediaMTX 进程生命周期（启动 / 停止 / 自恢复）
 *   5. 维护内部状态机，对外暴露运行状态与心跳信息
 *
 * 典型 JSON 配置示例：
 * @code
 * {
 *     "mediamtx_enable": true,
 *     "mediamtx_bin_path": "/path/to/mediamtx",
 *     "check_ip": "192.168.2.119",
 *     "yaml_file_name": "mediamtx.yaml",
 *     "mediamtx_json_config": {
 *         "rtspAddress": "8555",
 *         "paths": {
 *             "live": {
 *                 "source": "rtsp://192.168.2.119",
 *                 "sourceOnDemand": true
 *             }
 *         }
 *     }
 * }
 * @endcode
 */

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

namespace my_mediamtx_monitor {

/**
 * @brief RTSP 转发状态枚举
 */
enum class RtspRelayState {
    STOPPED,       ///< 未运行 / 未启用
    STARTING,      ///< MediaMTX 启动中
    RUNNING,       ///< 正常转发中
    SOURCE_LOST,   ///< RTSP 源不可达
    ERROR          ///< 异常
};

/**
 * @brief 状态枚举转可读字符串
 */
const char* RtspRelayStateToString(RtspRelayState state);

/**
 * @brief 将 nlohmann::json 转为 YAML 格式字符串（递归）
 * @param j       JSON 节点
 * @param indent  当前缩进层级（默认 0）
 * @return YAML 文本片段
 */
std::string JsonToYaml(const nlohmann::json& j, int indent = 0);

/**
 * @brief RTSP 转发守护类
 *
 * 负责监控 RTSP 源连通性并守护 MediaMTX 转发进程。
 * 内部维护一个独立线程，周期性执行：
 *   - RTSP 源可达性检测
 *   - MediaMTX 进程存活检测
 *   - 自动启动 / 停止 / 重启转发进程
 */
class RtspRelayMonitor {
public:
    RtspRelayMonitor();
    ~RtspRelayMonitor();

    // ======================== 禁止拷贝与移动 ========================
    RtspRelayMonitor(const RtspRelayMonitor&) = delete;
    RtspRelayMonitor& operator=(const RtspRelayMonitor&) = delete;

    /**
     * @brief 初始化模块
     * @param json_config JSON 配置（包含 mediamtx_enable、mediamtx_bin_path、check_ip、mediamtx_json_config 等）
     * @return true 配置解析和 YAML 生成均成功；false 失败
     */
    bool Init(const nlohmann::json& json_config);

    /**
     * @brief 启动监控线程
     * 
     * 调用前须先成功调用 Init()。
     * 线程启动后将周期性检测 RTSP 源并管理 MediaMTX 进程。
     */
    void Start();

    /**
     * @brief 停止监控线程并终止 MediaMTX 进程
     * 
     * 阻塞等待线程退出，保证资源完全释放。
     */
    void Stop();

    /**
     * @brief 获取当前状态
     */
    RtspRelayState GetState() const;

    /**
     * @brief 获取心跳 / 运行状态信息（JSON 格式）
     */
    nlohmann::json GetHeartbeat() const;

    /**
     * @brief 获取转发代理基本信息（在 Init 时收集）
     * @return JSON 对象，包含源信息、本机信息、拉流方式、帮助信息等
     */
    nlohmann::json GetInfo() const;

private:
    /* ======================= 核心线程 ======================= */

    /**
     * @brief 监控主循环
     *
     * 逻辑：
     *   1. enable_ == false → 停止进程，状态置 STOPPED
     *   2. RTSP 源不可达  → 停止进程，状态置 SOURCE_LOST
     *   3. 进程未运行      → 启动进程，状态置 STARTING
     *   4. 进程运行中      → 状态置 RUNNING
     */
    void MonitorLoop();

    /* ======================= 功能函数 ======================= */

    /**
     * @brief 检测 RTSP 源是否可达（TCP socket 连接 check_ip:rtsp_port）
     * @return true 可达；false 不可达
     */
    bool CheckRtspSource();

    /**
     * @brief 检测 MediaMTX 子进程是否仍存活
     * @return true 存活；false 已退出
     */
    bool CheckProcessAlive();

    /**
     * @brief 启动 MediaMTX 转发进程（fork + exec）
     * @return true 启动成功；false 失败
     */
    bool StartRelayProcess();

    /**
     * @brief 停止 MediaMTX 转发进程（SIGTERM → SIGKILL → waitpid）
     */
    void StopRelayProcess();

    /**
     * @brief 检测指定端口是否可用（未被占用）
     * @param port 端口号
     * @return true 可绑定（空闲）；false 已被占用
     */
    bool IsPortAvailable(int port);

    /**
     * @brief 从 JSON 配置生成 MediaMTX YAML 文件
     * @return true 写入成功；false 失败
     */
    bool GenerateYAML();

    /**
    * @brief 解析配置中的 RTSP 源地址与监听端口
     * @return true 解析成功；false 缺少必要字段
     */
    bool ParseConfig();

    /* ======================= 配置字段 ======================= */

    nlohmann::json json_config_;               ///< 原始 JSON 配置
    nlohmann::json mediamtx_json_config_;      ///< MediaMTX 专属 JSON 配置段
    std::string yaml_content_;                 ///< 生成的 YAML 字符串
    std::string local_ip_;                     ///< 本机 IP 地址（Init 时收集）
    std::string yaml_file_name_ = "mediamtx.yaml"; ///< YAML 文件名
    std::string yaml_file_abs_path_;           ///< YAML 文件绝对路径

    std::string mediamtx_bin_ = "./mediamtx";  ///< MediaMTX 可执行文件路径
    std::string check_ip_;                     ///< RTSP 源探测 IP
    int check_port_ = 2000;                    ///< RTSP 源探测端口（默认 2000）
    int rtsp_listen_port_ = 8555;              ///< MediaMTX 监听端口
    int monitor_interval_sec_ = 5;             ///< 监控轮询间隔（秒）

    /* ======================= 运行状态 ======================= */

    std::atomic<bool> enable_{false};          ///< 是否启用转发
    std::atomic<bool> running_{false};         ///< 监控线程运行标志
    bool config_initialized_ = false;          ///< 配置是否已成功初始化

    /* ======================= 进程管理 ======================= */

    pid_t relay_pid_{-1};                      ///< MediaMTX 子进程 PID

    /* ======================= 线程与同步 ======================= */

    std::thread monitor_thread_;               ///< 监控线程
    mutable std::mutex mutex_;                 ///< 保护共享状态的互斥锁

    /* ======================= 状态机 ======================= */

    RtspRelayState state_ = RtspRelayState::STOPPED; ///< 当前状态

    /* ======================= 基本信息 ======================= */

    nlohmann::json info_;                      ///< Init 时收集的转发代理基本信息
};

} // namespace my_mediamtx_monitor
