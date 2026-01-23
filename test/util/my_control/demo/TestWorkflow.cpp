#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "MyData.h"
#include "TaskQueue.h"
#include "demo/Workflow.h"
#include "demo/UUVControl.h"

using namespace my_control;
using namespace my_control::demo;

TEST(MyControl_Workflow, ExecuteOneTask) {
  TaskQueue q("workflow-queue");

  UUVControl ctrl;
  std::string err;
  ASSERT_TRUE(ctrl.Init(nlohmann::json{{"simulate_latency_ms", 1}}, &err));

  Workflow wf("wf-1", q, ctrl);

  std::atomic<bool> estop{false};
  wf.SetEStopFlag(&estop);

  std::atomic<int> finished{0};
  wf.SetFinishCallback([&](const my_data::Task&, const my_data::TaskResult& r) {
    if (r.code == my_data::ErrorCode::Ok) finished.fetch_add(1);
  });

  ASSERT_TRUE(wf.Start());

  my_data::Task t;
  t.task_id = "task-1";
  t.device_id = "uuv-1";
  t.capability = "navigate";
  t.action = "set";
  q.Push(t);

  // 等待执行完成
  for (int i = 0; i < 50 && finished.load() == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  EXPECT_EQ(finished.load(), 1);

  q.Shutdown();
  wf.Stop();
  wf.Join();
}