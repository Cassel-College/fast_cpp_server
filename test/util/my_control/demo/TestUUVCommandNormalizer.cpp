#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "MyData.h"
#include "MyLog.h"
#include "demo/uuv/UUVCommandNormalizer.h"

using namespace my_control::demo;

TEST(MyControl_UUVCommandNormalizer, NormalizeOk) {
  my_data::RawCommand rc;
  rc.command_id = "cmd-1";
  rc.source = "test";
  rc.received_at_ms = 1;
  rc.payload = nlohmann::json{
      {"device_id", "uuv-1"},
      {"capability", "navigate"},
      {"action", "set"},
      {"params", nlohmann::json{{"lat", 1.0}, {"lon", 2.0}}}
  };

  UUVCommandNormalizer n;
  std::string err;
  auto t = n.Normalize(rc, "edge-1", &err);
  ASSERT_TRUE(t.has_value());
  EXPECT_EQ(t->edge_id, "edge-1");
  EXPECT_EQ(t->device_id, "uuv-1");
  EXPECT_EQ(t->capability, "navigate");
  EXPECT_EQ(t->action, "set");
  EXPECT_FALSE(t->task_id.empty());
  EXPECT_FALSE(t->command_id.empty());
}

TEST(MyControl_UUVCommandNormalizer, NormalizeMissingField) {
  my_data::RawCommand rc;
  rc.payload = nlohmann::json{{"device_id", "uuv-1"}};

  UUVCommandNormalizer n;
  std::string err;
  auto t = n.Normalize(rc, "edge-1", &err);
  EXPECT_FALSE(t.has_value());
  EXPECT_FALSE(err.empty());
}