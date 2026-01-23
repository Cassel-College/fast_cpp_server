#include "MyControl.h"

#include "demo/UUVCommandNormalizer.h"
#include "demo/UUVControl.h"

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

  MYLOG_WARN("[MyControl] CreateNormalizer: unknown type={}, return nullptr", type);
  return nullptr;
}

std::unique_ptr<IControl> MyControl::CreateControl(const std::string& type) {
  MYLOG_INFO("[MyControl] CreateControl: type={}", type);

  if (type == "uuv" || type == "UUV") {
    return std::make_unique<my_control::demo::UUVControl>();
  }

  MYLOG_WARN("[MyControl] CreateControl: unknown type={}, return nullptr", type);
  return nullptr;
}

} // namespace my_control