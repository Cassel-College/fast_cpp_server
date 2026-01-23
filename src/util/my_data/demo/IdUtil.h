#pragma once
#include "Types.h"

namespace my_data {

/**
 * @brief 生成一个“够用”的ID（MVP 版）
 *
 * @note 这是 demo 级实现：时间戳 + 递增计数。
 *       未来可替换为 UUID，不影响上层。
 */
std::string GenerateId(const char* prefix);

} // namespace my_data