/**
 * @file pod_manager.cpp
 * @brief 吊舱管理器实现
 */

#include "pod_manager.h"
#include "../pod/dji/dji_pod.h"
#include "../pod/pinling/pinling_pod.h"
#include "MyLog.h"

namespace PodModule {

PodManager::PodManager() {
    MYLOG_INFO("[吊舱管理器] PodManager 构造");
}

PodManager::~PodManager() {
    MYLOG_INFO("[吊舱管理器] PodManager 析构");
}

bool PodManager::Init(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_.load()) {
        MYLOG_WARN("[吊舱管理器] 已经初始化过，跳过重复初始化");
        return true;
    }

    init_config_ = config;
    MYLOG_INFO("[吊舱管理器] 开始初始化，配置: {}", config.dump(4));

    // 解析 pod_args
    if (!config.contains("pod_args") || !config["pod_args"].is_object()) {
        MYLOG_WARN("[吊舱管理器] 配置中无 pod_args，初始化为空管理器");
        initialized_.store(true);
        return true;
    }

    const auto& pod_args = config["pod_args"];
    for (auto it = pod_args.begin(); it != pod_args.end(); ++it) {
        const std::string& pod_id = it.key();
        const auto& pod_cfg = it.value();

        auto pod = createPod(pod_id, pod_cfg);
        if (pod) {
            // 解析能力启用配置，设置后再装配能力
            auto enabled_caps = parseEnabledCapabilities(pod_cfg);
            pod->setEnabledCapabilities(enabled_caps);
            pod->initializeCapabilities();

            // 解析监控配置并按需自动启动
            auto monitor_cfg = parseMonitorConfig(pod_cfg);
            if (monitor_cfg.auto_start) {
                pod->startMonitor(monitor_cfg);
            }

            registry_.registerPod(pod_id, pod);
            MYLOG_INFO("[吊舱管理器] 初始化吊舱: {} ({})", pod_id, podVendorToString(pod->getVendor()));
        } else {
            MYLOG_ERROR("[吊舱管理器] 创建吊舱失败: {}", pod_id);
        }
    }

    initialized_.store(true);
    MYLOG_INFO("[吊舱管理器] 初始化完成，共 {} 个吊舱", registry_.size());
    return true;
}

nlohmann::json PodManager::GetInitConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return init_config_;
}

std::shared_ptr<IPod> PodManager::createPod(const std::string& pod_id, const nlohmann::json& pod_cfg) {
    std::string type = pod_cfg.value("type", "");
    std::string name = pod_cfg.value("name", pod_id);
    std::string ip   = pod_cfg.value("ip", "");
    int port         = pod_cfg.value("port", 0);

    PodInfo info;
    info.pod_id     = pod_id;
    info.pod_name   = name;
    info.ip_address = ip;
    info.port       = port;

    if (type == "dji") {
        info.vendor = PodVendor::DJI;
        return std::make_shared<DjiPod>(info);
    } else if (type == "pinling") {
        info.vendor = PodVendor::PINLING;
        return std::make_shared<PinlingPod>(info);
    } else {
        MYLOG_ERROR("[吊舱管理器] 未知吊舱类型: {}，pod_id={}", type, pod_id);
        return nullptr;
    }
}

PodResult<void> PodManager::addPod(std::shared_ptr<IPod> pod) {
    if (!pod) {
        MYLOG_ERROR("[吊舱管理器] 添加失败：吊舱实例为空");
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, "吊舱实例为空");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string pod_id = pod->getPodId();
    if (registry_.hasPod(pod_id)) {
        MYLOG_WARN("[吊舱管理器] 吊舱已存在: {}", pod_id);
        return PodResult<void>::fail(PodErrorCode::POD_ALREADY_EXISTS);
    }

    registry_.registerPod(pod_id, pod);
    MYLOG_INFO("[吊舱管理器] 添加吊舱成功: {} ({})",
               pod_id, podVendorToString(pod->getVendor()));
    return PodResult<void>::success("添加吊舱成功");
}

PodResult<void> PodManager::removePod(const std::string& pod_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!registry_.hasPod(pod_id)) {
        MYLOG_WARN("[吊舱管理器] 移除失败：吊舱不存在: {}", pod_id);
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND);
    }

    auto pod = registry_.getPod(pod_id);
    if (pod) {
        if (pod->isMonitorRunning()) {
            MYLOG_INFO("[吊舱管理器] 移除前停止监控: {}", pod_id);
            pod->stopMonitor();
        }
        if (pod->isConnected()) {
            MYLOG_INFO("[吊舱管理器] 移除前断开吊舱连接: {}", pod_id);
            pod->disconnect();
        }
    }

    registry_.unregisterPod(pod_id);
    MYLOG_INFO("[吊舱管理器] 移除吊舱成功: {}", pod_id);
    return PodResult<void>::success("移除吊舱成功");
}

std::shared_ptr<IPod> PodManager::getPod(const std::string& pod_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.getPod(pod_id);
}

std::vector<std::shared_ptr<IPod>> PodManager::listPods() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.listPods();
}

std::vector<std::string> PodManager::listPodIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.listPodIds();
}

size_t PodManager::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.size();
}

PodResult<void> PodManager::connectPod(const std::string& pod_id) {
    auto pod = getPod(pod_id);
    if (!pod) {
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND, "吊舱不存在: " + pod_id);
    }
    return pod->connect();
}

PodResult<void> PodManager::disconnectPod(const std::string& pod_id) {
    auto pod = getPod(pod_id);
    if (!pod) {
        return PodResult<void>::fail(PodErrorCode::POD_NOT_FOUND, "吊舱不存在: " + pod_id);
    }
    return pod->disconnect();
}

PodMonitorConfig PodManager::parseMonitorConfig(const nlohmann::json& pod_cfg) {
    PodMonitorConfig cfg;  // 所有字段已有默认值

    // 先从 capability 节点推导轮询开关：只轮询已启用的能力
    if (pod_cfg.contains("capability") && pod_cfg["capability"].is_object()) {
        auto isOpen = [&](const std::string& key) -> bool {
            if (!pod_cfg["capability"].contains(key)) return false;
            const auto& cap = pod_cfg["capability"][key];
            return cap.is_object() && cap.value("open", "") == "enable";
        };
        cfg.enable_status_poll = isOpen("STATUS");
        cfg.enable_ptz_poll    = isOpen("PTZ");
        cfg.enable_laser_poll  = isOpen("LASER");
        cfg.enable_stream_poll = isOpen("STREAM");
    }

    // monitor 节点可进一步覆盖上面的推导结果
    if (pod_cfg.contains("monitor") && pod_cfg["monitor"].is_object()) {
        const auto& m = pod_cfg["monitor"];

        if (m.contains("poll_interval_ms"))   cfg.poll_interval_ms   = m["poll_interval_ms"].get<uint32_t>();
        if (m.contains("status_interval_ms")) cfg.status_interval_ms = m["status_interval_ms"].get<uint32_t>();
        if (m.contains("ptz_interval_ms"))    cfg.ptz_interval_ms    = m["ptz_interval_ms"].get<uint32_t>();
        if (m.contains("laser_interval_ms"))  cfg.laser_interval_ms  = m["laser_interval_ms"].get<uint32_t>();
        if (m.contains("stream_interval_ms")) cfg.stream_interval_ms = m["stream_interval_ms"].get<uint32_t>();

        if (m.contains("online_window_size")) cfg.online_window_size = m["online_window_size"].get<uint32_t>();
        if (m.contains("online_threshold"))   cfg.online_threshold   = m["online_threshold"].get<uint32_t>();

        // monitor 节点的显式开关优先级最高，覆盖 capability 推导结果
        if (m.contains("enable_status_poll")) cfg.enable_status_poll = m["enable_status_poll"].get<bool>();
        if (m.contains("enable_ptz_poll"))    cfg.enable_ptz_poll    = m["enable_ptz_poll"].get<bool>();
        if (m.contains("enable_laser_poll"))  cfg.enable_laser_poll  = m["enable_laser_poll"].get<bool>();
        if (m.contains("enable_stream_poll")) cfg.enable_stream_poll = m["enable_stream_poll"].get<bool>();

        if (m.contains("auto_start"))         cfg.auto_start         = m["auto_start"].get<bool>();
    }

    return cfg;
}

std::set<CapabilityType> PodManager::parseEnabledCapabilities(const nlohmann::json& pod_cfg) {
    std::set<CapabilityType> enabled;

    if (!pod_cfg.contains("capability") || !pod_cfg["capability"].is_object()) {
        // 无 capability 配置，返回空集合 → BasePod 视为全部允许
        return enabled;
    }

    const auto& caps = pod_cfg["capability"];
    for (auto it = caps.begin(); it != caps.end(); ++it) {
        const std::string& key = it.key();
        const auto& val = it.value();

        if (!val.is_object()) continue;

        std::string open_val = val.value("open", "");
        if (open_val != "enable") continue;

        CapabilityType type;
        if (capabilityTypeFromString(key, type)) {
            enabled.insert(type);
            MYLOG_DEBUG("[吊舱管理器] 能力 {} 已启用", key);
        } else {
            MYLOG_WARN("[吊舱管理器] 未识别的能力类型: {}", key);
        }
    }

    MYLOG_INFO("[吊舱管理器] 从配置解析到 {} 个启用能力", enabled.size());
    return enabled;
}

nlohmann::json PodManager::GetStatusSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json result = nlohmann::json::array();

    auto pods = registry_.listPods();
    for (const auto& pod : pods) {
        nlohmann::json item;
        auto info = pod->getPodInfo();
        item["pod_id"]    = info.pod_id;
        item["pod_name"]  = info.pod_name;
        item["vendor"]    = podVendorToString(info.vendor);
        item["ip"]        = info.ip_address;
        item["port"]      = info.port;
        item["state"]     = podStateToString(pod->getState());
        item["connected"] = pod->isConnected();
        item["monitor_running"] = pod->isMonitorRunning();

        // 运行时状态快照
        if (pod->isMonitorRunning()) {
            auto rt = pod->getRuntimeStatus();
            item["is_online"]      = rt.is_online;
            item["last_update_ms"] = rt.last_update_ms;
        }

        // 列出已注册能力
        nlohmann::json caps = nlohmann::json::array();
        for (auto ct : pod->listCapabilities()) {
            caps.push_back(capabilityTypeToString(ct));
        }
        item["capabilities"] = caps;
        result.push_back(item);
    }
    return result;
}

} // namespace PodModule
