#include "MyDevice.h"

#include "demo/UUVDevice.h"

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

  MYLOG_WARN("[MyDevice] Create: unknown type={}, return nullptr", type);
  return nullptr;
}

} // namespace my_device