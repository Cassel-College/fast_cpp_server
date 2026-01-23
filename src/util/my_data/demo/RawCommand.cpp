#include "RawCommand.h"
#include "../JsonUtil.h"
#include <sstream>

namespace my_data {

std::string RawCommand::toString() const {
  std::ostringstream oss;
  oss << "RawCommand{command_id=" << command_id
      << ", source=" << source
      << ", received_at_ms=" << received_at_ms
      << ", idempotency_key=" << idempotency_key
      << ", dedup_window_ms=" << dedup_window_ms
      << ", payload=" << payload.dump()
      << "}";
  return oss.str();
}

nlohmann::json RawCommand::toJson() const {
  nlohmann::json j = nlohmann::json::object();
  j["command_id"] = command_id;
  j["source"] = source;
  j["received_at_ms"] = received_at_ms;
  j["idempotency_key"] = idempotency_key;
  j["dedup_window_ms"] = dedup_window_ms;
  j["payload"] = payload;
  return j;
}

RawCommand RawCommand::fromJson(const nlohmann::json& j) {
  RawCommand c;
  c.command_id = my_data::jsonutil::GetStringOr(j, "command_id", "");
  c.source = my_data::jsonutil::GetStringOr(j, "source", "");
  c.received_at_ms = my_data::jsonutil::GetInt64Or(j, "received_at_ms", 0);
  c.idempotency_key = my_data::jsonutil::GetStringOr(j, "idempotency_key", "");
  c.dedup_window_ms = my_data::jsonutil::GetInt64Or(j, "dedup_window_ms", 0);

  auto pit = j.find("payload");
  if (pit != j.end() && pit->is_object()) c.payload = *pit;
  return c;
}

} // namespace my_data