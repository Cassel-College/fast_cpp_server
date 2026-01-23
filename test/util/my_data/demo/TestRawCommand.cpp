#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "MyData.h"

using namespace my_data;

TEST(MyData_RawCommand, JsonRoundTrip) {
  RawCommand c;
  c.command_id = "cmd-1";
  c.source = "mqtt";
  c.received_at_ms = 123;
  c.idempotency_key = "cmd-1";
  c.dedup_window_ms = 5000;
  c.payload = nlohmann::json{{"device_id", "dev-1"}, {"action", "start"}};

  auto j = c.toJson();
  auto c2 = RawCommand::fromJson(j);

  EXPECT_EQ(c2.command_id, c.command_id);
  EXPECT_EQ(c2.source, c.source);
  EXPECT_EQ(c2.idempotency_key, "cmd-1");
  EXPECT_TRUE(c2.payload.contains("device_id"));
  EXPECT_FALSE(c2.toString().empty());
}