#include "TaskResult.h"
#include "../JsonUtil.h"
#include <sstream>

namespace my_data {

std::string TaskResult::toString() const {
  std::ostringstream oss;
  oss << "TaskResult{code=" << my_data::ToString(code)
      << ", message=" << message
      << ", started_at_ms=" << started_at_ms
      << ", finished_at_ms=" << finished_at_ms
      << ", output=" << output.dump()
      << "}";
  return oss.str();
}

nlohmann::json TaskResult::toJson() const {
  nlohmann::json j = nlohmann::json::object();
  j["code"] = static_cast<int>(code);
  j["message"] = message;
  j["started_at_ms"] = started_at_ms;
  j["finished_at_ms"] = finished_at_ms;
  j["output"] = output;
  return j;
}

TaskResult TaskResult::fromJson(const nlohmann::json& j) {
  TaskResult r;
  int code_i = my_data::jsonutil::GetIntOr(j, "code", static_cast<int>(ErrorCode::Ok));
  r.code = static_cast<ErrorCode>(code_i);
  r.message = my_data::jsonutil::GetStringOr(j, "message", "");
  r.started_at_ms = my_data::jsonutil::GetInt64Or(j, "started_at_ms", 0);
  r.finished_at_ms = my_data::jsonutil::GetInt64Or(j, "finished_at_ms", 0);
  auto it = j.find("output");
  if (it != j.end() && it->is_object()) r.output = *it;
  return r;
}

} // namespace my_data