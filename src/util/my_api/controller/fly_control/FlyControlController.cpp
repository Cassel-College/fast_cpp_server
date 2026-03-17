#include "FlyControlController.h"

#include "MyFlyControlManager.h"
#include "MyLog.h"

namespace my_api::fly_control_api {

using namespace my_api::base;

namespace {

nlohmann::json FaultBitsToJson(const ::fly_control::FaultBits& bits) {
    return {
        {"link_lost", bits.link_lost},
        {"distance_exceed", bits.distance_exceed},
        {"attitude_exceed", bits.attitude_exceed},
        {"altitude_too_low", bits.altitude_too_low},
        {"altitude_too_high", bits.altitude_too_high},
        {"voltage_too_low", bits.voltage_too_low},
        {"mission_fault", bits.mission_fault},
        {"gps_lost", bits.gps_lost},
        {"takeoff_failed", bits.takeoff_failed},
        {"attitude_error", bits.attitude_error},
        {"return_timeout", bits.return_timeout},
        {"return_dist_error", bits.return_dist_error},
        {"parachute_failed", bits.parachute_failed}
    };
}

nlohmann::json BuildFlyControlSnapshot() {
    auto& manager = ::fly_control::MyFlyControlManager::GetInstance();

    const bool initialized = manager.IsInitialized();
    const bool running = manager.IsRunning();
    const bool has_heartbeat = manager.HasHeartbeat();

    nlohmann::json data;
    data["initialized"] = initialized;
    data["running"] = running;
    data["has_heartbeat"] = has_heartbeat;
    data["fault_bits"] = FaultBitsToJson(manager.GetFaultBits());
    data["heartbeat"] = has_heartbeat ? manager.GetLatestHeartbeatJson() : nlohmann::json::object();
    return data;
}

} // namespace

FlyControlController::FlyControlController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : BaseApiController(objectMapper) {}

std::shared_ptr<FlyControlController> FlyControlController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper) {
    return std::make_shared<FlyControlController>(objectMapper);
}

MyAPIResponsePtr FlyControlController::getFlyControlOnline() {
    MYLOG_INFO("[API] FlyControl GET online");
    return jsonOk({{"status", "alive"}}, "fly control api alive");
}

MyAPIResponsePtr FlyControlController::getFlyControlStatus() {
    MYLOG_INFO("[API] FlyControl GET status");
    return jsonOk(BuildFlyControlSnapshot(), "fly control status retrieved");
}

MyAPIResponsePtr FlyControlController::getFlyControlHeartbeatData() {
    MYLOG_INFO("[API] FlyControl GET heartbeat data");

    auto& manager = ::fly_control::MyFlyControlManager::GetInstance();
    const bool has_heartbeat = manager.HasHeartbeat();

    nlohmann::json data;
    data["has_heartbeat"] = has_heartbeat;
    data["heartbeat"] = has_heartbeat ? manager.GetLatestHeartbeatJson() : nlohmann::json::object();

    return jsonOk(data, has_heartbeat ? "fly control heartbeat retrieved"
                                      : "fly control heartbeat not received yet");
}

} // namespace my_api::fly_control_api