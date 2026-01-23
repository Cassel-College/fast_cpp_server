#include <gtest/gtest.h>

#include "MyData.h"
#include "MyLog.h"
#include "TaskQueue.h"

using namespace my_control;

TEST(MyControl_TaskQueue, PushPopBlocking) {
  TaskQueue q("test-queue");

  my_data::Task t;
  t.task_id = "task-1";
  t.device_id = "dev-1";
  t.capability = "navigate";
  t.action = "set";

  q.Push(t);

  my_data::Task out;
  bool ok = q.PopBlocking(out, 10);
  EXPECT_TRUE(ok);
  EXPECT_EQ(out.task_id, "task-1");
  EXPECT_EQ(q.Size(), 0u);
}

TEST(MyControl_TaskQueue, ShutdownWakeUp) {
  TaskQueue q("test-queue-shutdown");
  q.Shutdown();

  my_data::Task out;
  bool ok = q.PopBlocking(out, 10);
  EXPECT_FALSE(ok);
  EXPECT_TRUE(q.IsShutdown());
}