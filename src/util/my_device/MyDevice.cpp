#include "MyDevice.h"

#include "demo/uuv/UUVDevice.h"
#include "demo/depth_sensor/DepthSensorDevice.h"
#include "demo/flow_sensor/FlowSensorDevice.h"
#include "demo/wind_sensor/WindSensorDevice.h"

namespace my_device {

MyDevice& MyDevice::GetInstance() {
  static MyDevice inst;
  return inst;
}

std::unique_ptr<IDevice> MyDevice::Create(const std::string& type) {
  MYLOG_INFO("[MyDevice] Create: type={}", type);

  if (type == "uuv" || type == "UUV") {
    return std::make_unique<my_device::demo::UUVDevice>();
  }
  if (type == "depth_sensor") {
    return std::make_unique<my_device::demo::DepthSensorDevice>();
  }
  if (type == "flow_sensor") {
    return std::make_unique<my_device::demo::FlowSensorDevice>();
  }
  if (type == "split_speed_sensor") {
    return std::make_unique<my_device::demo::WindSensorDevice>();
  }

  MYLOG_WARN("[MyDevice] Create: unknown type={}, return nullptr", type);
  return nullptr;
}


std::unique_ptr<IDevice> MyDevice::Create(const std::string& type, const nlohmann::json& cfg, std::string* err) {
  MYLOG_INFO("[MyDevice] Create with cfg: type={}", type);

  if (type == "uuv" || type == "UUV") {
    return std::make_unique<my_device::demo::UUVDevice>(cfg, err);
  }
  if (type == "depth_sensor") {
    return std::make_unique<my_device::demo::DepthSensorDevice>(cfg, err);
  }
  if (type == "flow_sensor") {
    return std::make_unique<my_device::demo::FlowSensorDevice>(cfg, err);
  }
  if (type == "split_speed_sensor") {
    return std::make_unique<my_device::demo::WindSensorDevice>(cfg, err);
  }

  MYLOG_WARN("[MyDevice] Create with cfg: unknown type={}, return nullptr", type);
  return nullptr;
};

} // namespace my_device