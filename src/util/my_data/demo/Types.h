#pragma once
#include <cstdint>
#include <string>

namespace my_data {

/**
 * @brief 边缘执行器ID
 */
using EdgeId = std::string;

/**
 * @brief 专业设备ID（按 device_id 一对一实例）
 */
using DeviceId = std::string;

/**
 * @brief Task ID
 */
using TaskId = std::string;

/**
 * @brief Command ID（外部输入或上层生成，用于溯源/幂等）
 */
using CommandId = std::string;

/**
 * @brief 毫秒时间戳
 */
using TimestampMs = std::int64_t;

} // namespace my_data