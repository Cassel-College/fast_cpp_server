#include <gtest/gtest.h>
#include "MyData.h"

using namespace my_data;

TEST(MyData_Status, DeviceStatusJsonRoundTrip) {
  DeviceStatus ds;
  ds.device_id = "dev-1";
  ds.conn_state = DeviceConnState::Online;
  ds.work_state = DeviceWorkState::Busy;
  ds.running_task_id = "task-9";
  ds.last_task_at_ms = 100;
  ds.last_seen_at_ms = 200;
  ds.last_error = "none";
  ds.queue_depth = 3;

  auto j = ds.toJson();
  auto ds2 = DeviceStatus::fromJson(j);

  EXPECT_EQ(ds2.device_id, ds.device_id);
  EXPECT_EQ(static_cast<int>(ds2.conn_state), static_cast<int>(ds.conn_state));
  EXPECT_EQ(static_cast<int>(ds2.work_state), static_cast<int>(ds.work_state));
  EXPECT_EQ(ds2.queue_depth, 3);
  EXPECT_FALSE(ds2.toString().empty());
}

TEST(MyData_Status, EdgeStatusJsonRoundTrip) {
  EdgeStatus es;
  es.edge_id = "edge-1";
  es.run_state = EdgeRunState::Running;
  es.boot_at_ms = 1;
  es.last_heartbeat_at_ms = 2;
  es.estop_active = false;
  es.tasks_pending_total = 10;
  es.tasks_running_total = 1;

  DeviceStatus ds;
  ds.device_id = "dev-1";
  ds.conn_state = DeviceConnState::Online;
  es.devices.emplace(ds.device_id, ds);

  auto j = es.toJson();
  auto es2 = EdgeStatus::fromJson(j);

  EXPECT_EQ(es2.edge_id, es.edge_id);
  EXPECT_EQ(static_cast<int>(es2.run_state), static_cast<int>(es.run_state));
  EXPECT_EQ(es2.devices.size(), 1u);
  EXPECT_TRUE(es2.devices.find("dev-1") != es2.devices.end());
  EXPECT_FALSE(es2.toString().empty());
}