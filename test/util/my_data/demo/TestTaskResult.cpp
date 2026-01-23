#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "MyData.h"

using namespace my_data;

TEST(MyData_TaskResult, JsonRoundTrip) {
  TaskResult r;
  r.code = ErrorCode::DriverError;
  r.message = "port closed";
  r.started_at_ms = 10;
  r.finished_at_ms = 20;
  r.output = nlohmann::json{{"detail", "x"}};

  auto j = r.toJson();
  auto r2 = TaskResult::fromJson(j);

  EXPECT_EQ(static_cast<int>(r2.code), static_cast<int>(r.code));
  EXPECT_EQ(r2.message, r.message);
  EXPECT_TRUE(r2.output.contains("detail"));
  EXPECT_FALSE(r2.toString().empty());
}