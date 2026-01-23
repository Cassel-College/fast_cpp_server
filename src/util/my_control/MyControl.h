#pragma once

#include <memory>
#include <string>

#include "ICommandNormalizer.h"
#include "IControl.h"
#include "MyLog.h"

namespace my_control {

/**
 * @brief my_control 门面（轻量）
 *
 * @details
 * - 由于队列实例在 Edge，本类不负责创建/持有 TaskQueue
 * - 提供：创建 demo normalizer/control 的工厂方法
 * - 未来可扩展为注册表（按 type 返回不同实现）
 */
class MyControl {
public:
  static MyControl& GetInstance();

  std::unique_ptr<ICommandNormalizer> CreateNormalizer(const std::string& type);
  std::unique_ptr<IControl> CreateControl(const std::string& type);

private:
  MyControl() = default;
};

} // namespace my_control