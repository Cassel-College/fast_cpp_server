#include "MyFlyControlManager.h"

namespace fly_control {

MyFlyControlManager::MyFlyControlManager()
    : controller_(std::make_shared<MyFlyControl>()) {}

MyFlyControlManager::~MyFlyControlManager() {
    std::shared_ptr<MyFlyControl> old_controller;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        old_controller = std::move(controller_);
        initialized_ = false;
    }

    if (old_controller) {
        old_controller->Stop();
    }
}

MyFlyControlManager& MyFlyControlManager::GetInstance() {
    static MyFlyControlManager instance;
    return instance;
}

bool MyFlyControlManager::Init(const nlohmann::json& cfg, std::string* err) {
    std::shared_ptr<MyFlyControl> controller;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!controller_) {
            controller_ = std::make_shared<MyFlyControl>();
        }
        if (controller_->IsRunning()) {
            if (err) {
                *err = "飞控正在运行，请先调用 Stop 后再重新初始化";
            }
            return false;
        }
        controller = controller_;
    }

    if (!controller->Init(cfg, err)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (controller_ == controller) {
            initialized_ = true;
        }
    }
    return true;
}

bool MyFlyControlManager::Start(std::string* err) {
    auto controller = GetInitializedController(err);
    if (!controller) {
        return false;
    }
    return controller->Start(err);
}

void MyFlyControlManager::Stop() {
    auto controller = GetControllerSnapshot();
    if (controller) {
        controller->Stop();
    }
}

void MyFlyControlManager::Shutdown() {
    std::shared_ptr<MyFlyControl> old_controller;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        old_controller = controller_;
        controller_ = std::make_shared<MyFlyControl>();
        initialized_ = false;
    }

    if (old_controller) {
        old_controller->Stop();
    }
}

bool MyFlyControlManager::IsInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

bool MyFlyControlManager::IsRunning() const {
    auto controller = GetControllerSnapshot();
    return controller && controller->IsRunning();
}

void MyFlyControlManager::SetOnHeartbeat(HeartbeatCallback cb) {
    auto controller = GetControllerSnapshot();
    if (controller) {
        controller->SetOnHeartbeat(std::move(cb));
    }
}

void MyFlyControlManager::SetOnCommandReply(CommandReplyCallback cb) {
    auto controller = GetControllerSnapshot();
    if (controller) {
        controller->SetOnCommandReply(std::move(cb));
    }
}

void MyFlyControlManager::SetOnGimbalControl(GimbalControlCallback cb) {
    auto controller = GetControllerSnapshot();
    if (controller) {
        controller->SetOnGimbalControl(std::move(cb));
    }
}

HeartbeatData MyFlyControlManager::GetLatestHeartbeat() const {
    auto controller = GetControllerSnapshot();
    return controller ? controller->GetLatestHeartbeat() : HeartbeatData{};
}

nlohmann::json MyFlyControlManager::GetLatestHeartbeatJson() const {
    auto controller = GetControllerSnapshot();
    return controller ? controller->GetLatestHeartbeatJson() : nlohmann::json::object();
}

FaultBits MyFlyControlManager::GetFaultBits() const {
    auto controller = GetControllerSnapshot();
    return controller ? controller->GetFaultBits() : FaultBits::FromU16(0);
}

bool MyFlyControlManager::HasHeartbeat() const {
    auto controller = GetControllerSnapshot();
    return controller && controller->HasHeartbeat();
}

bool MyFlyControlManager::SendSetDestination(int32_t lon, int32_t lat, uint16_t alt,
                                             std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetDestination(lon, lat, alt, err) : false;
}

bool MyFlyControlManager::SendSetRoute(const std::vector<RoutePoint>& points, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetRoute(points, err) : false;
}

bool MyFlyControlManager::SendSetAngle(int16_t pitch, uint16_t yaw, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetAngle(pitch, yaw, err) : false;
}

bool MyFlyControlManager::SendSetSpeed(uint16_t speed, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetSpeed(speed, err) : false;
}

bool MyFlyControlManager::SendSetAltitude(uint8_t alt_type, uint16_t altitude,
                                          std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetAltitude(alt_type, altitude, err) : false;
}

bool MyFlyControlManager::SendPowerSwitch(uint8_t command, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendPowerSwitch(command, err) : false;
}

bool MyFlyControlManager::SendParachute(uint8_t parachute_type, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendParachute(parachute_type, err) : false;
}

bool MyFlyControlManager::SendButtonCommand(uint8_t button, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendButtonCommand(button, err) : false;
}

bool MyFlyControlManager::SendSetOriginReturn(uint8_t point_type, int32_t lon, int32_t lat,
                                              uint16_t alt, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetOriginReturn(point_type, lon, lat, alt, err) : false;
}

bool MyFlyControlManager::SendSetGeofence(const std::vector<GeofencePoint>& points,
                                          std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSetGeofence(points, err) : false;
}

bool MyFlyControlManager::SendSwitchMode(uint8_t mode, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendSwitchMode(mode, err) : false;
}

bool MyFlyControlManager::SendGuidance(const GuidanceData& data, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendGuidance(data, err) : false;
}

bool MyFlyControlManager::SendGuidanceNew(const GuidanceNewData& data, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendGuidanceNew(data, err) : false;
}

bool MyFlyControlManager::SendGimbalAngRate(const GimbalAngRateData& data, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendGimbalAngRate(data, err) : false;
}

bool MyFlyControlManager::SendGimbalAngle(const GimbalAngleData& data, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendGimbalAngle(data, err) : false;
}

bool MyFlyControlManager::SendTargetState(const TargetStateData& data, std::string* err) {
    auto controller = GetInitializedController(err);
    return controller ? controller->SendTargetState(data, err) : false;
}

std::shared_ptr<MyFlyControl> MyFlyControlManager::GetControllerSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return controller_;
}

std::shared_ptr<MyFlyControl> MyFlyControlManager::GetInitializedController(std::string* err) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || !controller_) {
        if (err) {
            *err = "飞控管理器尚未初始化，请先调用 Init";
        }
        return nullptr;
    }
    return controller_;
}

} // namespace fly_control