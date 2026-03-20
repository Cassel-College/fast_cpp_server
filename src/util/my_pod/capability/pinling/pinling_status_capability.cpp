/**
 * @file pinling_status_capability.cpp
 * @brief PINLING状态查询能力实现
 */

#include "pinling_status_capability.h"
#include "MyLog.h"
#include "../../pod/interface/i_pod.h"
#include "../../../my_tools/PingTools.h"

namespace PodModule {

PinlingStatusCapability::PinlingStatusCapability() {
    MYLOG_DEBUG("[PINLING能力] PinlingStatusCapability 构造");
}

PinlingStatusCapability::~PinlingStatusCapability() {
    MYLOG_DEBUG("[PINLING能力] PinlingStatusCapability 析构");
}

PodResult<PodStatus> PinlingStatusCapability::getStatus() {
    MYLOG_DEBUG("{} 获取PINLING设备状态", logPrefix());

    auto refresh_result = refreshStatus();
    if (!refresh_result.isSuccess()) {
        return PodResult<PodStatus>::fail(refresh_result.code, refresh_result.message);
    }

    const bool is_online = cached_status_.state == PodState::CONNECTED;
    return PodResult<PodStatus>::success(
        cached_status_,
        is_online ? "通过 Ping 探测判定PINLING设备在线" : "通过 Ping 探测判定PINLING设备离线");
}

PodResult<void> PinlingStatusCapability::refreshStatus() {
    MYLOG_INFO("{} 刷新PINLING设备状态", logPrefix());

    auto* pod = getPod();
    if (!pod) {
        cached_status_.state = PodState::ERROR;
        cached_status_.temperature = 0.0;
        cached_status_.voltage = 0.0;
        cached_status_.uptime_ms = 0;
        cached_status_.error_msg = "PINLING状态能力未关联 Pod";
        MYLOG_ERROR("{} {}", logPrefix(), cached_status_.error_msg);
        return PodResult<void>::fail(PodErrorCode::CAPABILITY_INIT_FAILED, cached_status_.error_msg);
    }

    const auto pod_info = pod->getPodInfo();
    if (pod_info.ip_address.empty()) {
        cached_status_.state = PodState::ERROR;
        cached_status_.temperature = 0.0;
        cached_status_.voltage = 0.0;
        cached_status_.uptime_ms = 0;
        cached_status_.error_msg = "PINLING设备 IP 未配置";
        MYLOG_ERROR("{} {}", logPrefix(), cached_status_.error_msg);
        return PodResult<void>::fail(PodErrorCode::UNKNOWN_ERROR, cached_status_.error_msg);
    }

    const bool is_online = my_tools::ping_tools::PingFuncBySystem::PingIP(pod_info.ip_address, 1, 1);
    cached_status_.state = is_online ? PodState::CONNECTED : PodState::DISCONNECTED;
    cached_status_.temperature = 0.0;
    cached_status_.voltage = 0.0;
    cached_status_.uptime_ms = 0;
    cached_status_.error_msg = is_online ? "" : "Ping 探测失败，设备不可达";

    MYLOG_INFO("{} PINLING设备 {} 在线状态: {}",
               logPrefix(),
               pod_info.ip_address,
               is_online ? "在线" : "离线");

    return PodResult<void>::success(
        is_online ? "通过 Ping 探测判定PINLING设备在线" : "通过 Ping 探测判定PINLING设备离线");
}

} // namespace PodModule
