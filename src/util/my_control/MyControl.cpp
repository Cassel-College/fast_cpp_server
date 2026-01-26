#include "MyControl.h"

#include "demo/uuv/UUVCommandNormalizer.h"
#include "demo/uuv/UUVControl.h"

#include "demo/depth_sensor/DepthSensorCommandNormalizer.h"
#include "demo/flow_sensor/FlowSensorCommandNormalizer.h"
#include "demo/wind_sensor/WindSensorCommandNormalizer.h"

#include "demo/depth_sensor/DepthSensorControl.h"
#include "demo/flow_sensor/FlowSensorControl.h"
#include "demo/wind_sensor/WindSensorDevice.h"

namespace my_control {

MyControl& MyControl::GetInstance() {
  static MyControl inst;
  return inst;
}

std::unique_ptr<ICommandNormalizer> MyControl::CreateNormalizer(const std::string& type) {
  MYLOG_INFO("[MyControl] CreateNormalizer: type={}", type);

  if (type == "uuv" || type == "UUV") {
    return std::make_unique<my_control::demo::UUVCommandNormalizer>();
  }
  if (type == "depth_sensor") {
    return std::make_unique<my_control::demo::DepthSensorCommandNormalizer>();
  }
  if (type == "flow_sensor") {
    return std::make_unique<my_control::demo::FlowSensorCommandNormalizer>();
  }
  if (type == "wind_sensor") {
    return std::make_unique<my_control::demo::WindSensorCommandNormalizer>();
  }

  MYLOG_WARN("[MyControl] CreateNormalizer: unknown type={}, return nullptr", type);
  return nullptr;
}

std::unique_ptr<IControl> MyControl::CreateControl(const std::string& type) {
  MYLOG_INFO("[MyControl] CreateControl: type={}", type);

  if (type == "uuv" || type == "UUV") {
    return std::make_unique<my_control::demo::UUVControl>();
  }
  if (type == "depth_sensor") {
    return std::make_unique<my_control::demo::DepthSensorControl>();
  }
  if (type == "flow_sensor") {
    return std::make_unique<my_control::demo::FlowSensorControl>();
  }
  if (type == "wind_sensor") {
    return std::make_unique<my_control::demo::WindSensorDevice>();
  }

  MYLOG_WARN("[MyControl] CreateControl: unknown type={}, return nullptr", type);
  return nullptr;
}

} // namespace my_control