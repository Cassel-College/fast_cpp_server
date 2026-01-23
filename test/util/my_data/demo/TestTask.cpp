#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "MyData.h"

using namespace my_data;

TEST(MyData_Task, JsonRoundTrip) {
  Task t;
  t.task_id = "task-1";
  t.command_id = "cmd-1";
  t.edge_id = "edge-1";
  t.device_id = "dev-1";
  t.capability = "spray";
  t.action = "start";
  t.params = nlohmann::json{{"rate", 0.8}, {"width", 3.0}};
  t.idempotency_key = "cmd-1";
  t.dedup_window_ms = 60000;
  t.priority = 1;
  t.created_at_ms = 123;
  t.state = TaskState::Pending;

  auto j = t.toJson();
  auto t2 = Task::fromJson(j);

  EXPECT_EQ(t2.task_id, t.task_id);
  EXPECT_EQ(t2.command_id, t.command_id);
  EXPECT_EQ(t2.device_id, t.device_id);
  EXPECT_EQ(t2.capability, t.capability);
  EXPECT_EQ(t2.action, t.action);
  EXPECT_TRUE(t2.params.contains("rate"));
  EXPECT_EQ(t2.idempotency_key, "cmd-1");
  EXPECT_EQ(t2.dedup_window_ms, 60000);
  EXPECT_FALSE(t2.toString().empty());
}