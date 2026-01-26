#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "MyData.h"
#include "demo/uuv/UUVControl.h"

using namespace my_control::demo;

TEST(MyControl_UUVControl, DoTaskNavigateSet) {
  UUVControl c;
  std::string err;
  EXPECT_TRUE(c.Init(nlohmann::json{{"device_name", "uuv-1"}, {"simulate_latency_ms", 1}}, &err));

  my_data::Task t;
  t.task_id = "task-1";
  t.device_id = "uuv-1";
  t.capability = "navigate";
  t.action = "set";
  t.params = nlohmann::json{{"lat", 1.0}, {"lon", 2.0}};

  auto r = c.DoTask(t);
  EXPECT_EQ(r.code, my_data::ErrorCode::Ok);
}

TEST(MyControl_UUVControl, DoTaskUnknownCapability) {
  UUVControl c;
  std::string err;
  EXPECT_TRUE(c.Init(nlohmann::json{{"simulate_latency_ms", 1}}, &err));

  my_data::Task t;
  t.task_id = "task-2";
  t.device_id = "uuv-1";
  t.capability = "unknown";
  t.action = "x";

  auto r = c.DoTask(t);
  EXPECT_EQ(r.code, my_data::ErrorCode::InvalidCommand);
}