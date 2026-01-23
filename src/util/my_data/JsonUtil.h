#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace my_data::jsonutil {

/**
 * @brief 从 json 中读取 string，key 不存在或为 null 则返回默认值
 */
std::string GetStringOr(const nlohmann::json& j, const char* key, const std::string& def);

/**
 * @brief 从 json 中读取 int，key 不存在或为 null 则返回默认值
 */
int GetIntOr(const nlohmann::json& j, const char* key, int def);

/**
 * @brief 从 json 中读取 int64，key 不存在或为 null 则返回默认值
 */
std::int64_t GetInt64Or(const nlohmann::json& j, const char* key, std::int64_t def);

} // namespace my_data::jsonutil