#pragma once

#include <optional>
#include <string>

#include "ICommandNormalizer.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_control::demo {

/**
 * @brief UUV 命令规范化器（demo）
 *
 * @details
 * 期望 RawCommand.payload 格式（示例）：
 * {
 *   "device_id": "uuv-1",
 *   "capability": "navigate",
 *   "action": "set",
 *   "params": { "lat": 1.23, "lon": 4.56, "depth": 12.0 }
 * }
 */
class UUVCommandNormalizer final : public my_control::ICommandNormalizer {
public:
  std::optional<my_data::Task> Normalize(const my_data::RawCommand& in,
                                         const my_data::EdgeId& edge_id,
                                         std::string* err) override;

  std::string Name() const override { return "UUVCommandNormalizer"; }
};

} // namespace my_control::demo