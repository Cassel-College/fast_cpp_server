#include "JsonUtil.h"

namespace my_data::jsonutil {

std::string GetStringOr(const nlohmann::json& j, const char* key, const std::string& def) {
  if (!key) return def;
  auto it = j.find(key);
  if (it == j.end() || it->is_null()) return def;
  if (!it->is_string()) return def;
  return it->get<std::string>();
}

int GetIntOr(const nlohmann::json& j, const char* key, int def) {
  if (!key) return def;
  auto it = j.find(key);
  if (it == j.end() || it->is_null()) return def;
  if (it->is_number_integer()) return it->get<int>();
  return def;
}

std::int64_t GetInt64Or(const nlohmann::json& j, const char* key, std::int64_t def) {
  if (!key) return def;
  auto it = j.find(key);
  if (it == j.end() || it->is_null()) return def;
  if (it->is_number_integer()) return it->get<std::int64_t>();
  return def;
}

} // namespace my_data::jsonutil