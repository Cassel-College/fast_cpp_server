#include "MediamtxMonitorController.h"
#include "RtspRelayMonitorManager.h"
#include "BaseApiController.hpp"
#include "MyLog.h"

namespace my_api::mediamtx_monitor_api {

using namespace my_api::base;
using namespace my_mediamtx_monitor;

MediamtxMonitorController::MediamtxMonitorController(
    const std::shared_ptr<ObjectMapper>& objectMapper
) : BaseApiController(objectMapper) {}

std::shared_ptr<MediamtxMonitorController>
MediamtxMonitorController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper
) {
    return std::make_shared<MediamtxMonitorController>(objectMapper);
}

// GET /v1/mediamtx/online
MyAPIResponsePtr MediamtxMonitorController::getOnline() {
    MYLOG_INFO("[API] MediaMTX Monitor GET online");
    return jsonOk({{"alive", true}}, "MediaMTX Monitor is alive");
}

// GET /v1/mediamtx/status
MyAPIResponsePtr MediamtxMonitorController::getStatus() {
    MYLOG_INFO("[API] MediaMTX Monitor GET status");
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    nlohmann::json data;
    data["state"] = RtspRelayStateToString(mgr.GetState());
    data["running"] = mgr.IsRunning();
    data["heartbeat"] = mgr.GetHeartbeat();
    return jsonOk(data, "OK");
}

// POST /v1/mediamtx/start
MyAPIResponsePtr MediamtxMonitorController::postStart() {
    MYLOG_INFO("[API] MediaMTX Monitor POST start");
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    if (mgr.IsRunning()) {
        return jsonOk({{"already_running", true}}, "MediaMTX Monitor is already running");
    }
    mgr.Start();
    return jsonOk({{"started", true}}, "MediaMTX Monitor started");
}

// POST /v1/mediamtx/stop
MyAPIResponsePtr MediamtxMonitorController::postStop() {
    MYLOG_INFO("[API] MediaMTX Monitor POST stop");
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    if (!mgr.IsRunning()) {
        return jsonOk({{"already_stopped", true}}, "MediaMTX Monitor is not running");
    }
    mgr.Stop();
    return jsonOk({{"stopped", true}}, "MediaMTX Monitor stopped");
}

// GET /v1/mediamtx/info
MyAPIResponsePtr MediamtxMonitorController::getInfo() {
    MYLOG_INFO("[API] MediaMTX Monitor GET info");
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    nlohmann::json info = mgr.GetInfo();
    if (info.is_null() || info.empty()) {
        return jsonError(404, "MediaMTX Monitor 尚未初始化，无可用信息");
    }
    return jsonOk(info, "OK");
}

} // namespace my_api::mediamtx_monitor_api
