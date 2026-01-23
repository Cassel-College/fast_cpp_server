#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "MyData.h"
#include "MyLog.h"

namespace my_control {

/**
 * @brief 命令规范化接口：RawCommand/json -> Task
 *
 * @details
 * 主要职责：
 * - 解析 payload
 * - 校验必填字段
 * - 补齐 task_id/created_at_ms/command_id/idempotency_key 等默认值
 */
class ICommandNormalizer {
public:
  virtual ~ICommandNormalizer() = default;

  /**
   * @brief 将 RawCommand 转为 Task
   * @param in 原始命令
   * @param edge_id 当前���缘设备ID（用于补齐 Task.edge_id）
   * @param err 输出错误信息（可选）
   * @return 成功返回 Task，失败返回 nullopt
   */
  virtual std::optional<my_data::Task> Normalize(const my_data::RawCommand& in,
                                                 const my_data::EdgeId& edge_id,
                                                 std::string* err) = 0;

  virtual std::string Name() const { return "ICommandNormalizer"; }
};

} // namespace my_control