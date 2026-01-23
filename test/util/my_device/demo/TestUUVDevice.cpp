#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "MyData.h"
#include "MyLog.h"
#include "TaskQueue.h"
#include "MyDevice.h"

using namespace my_device;

TEST(MyDevice_UUVDevice, InitStartStop) {
  auto dev = MyDevice::GetInstance().Create("uuv");
  ASSERT_TRUE(dev != nullptr);

  std::string err;
  nlohmann::json cfg = {
      {"device_id", "uuv-1"},
      {"device_name", "UUV-ONE"},
      {"control", nlohmann::json{{"simulate_latency_ms", 1}}}
  };

  ASSERT_TRUE(dev->Init(cfg, &err)) << err;

  my_control::TaskQueue q("queue-uuv-1");
  std::atomic<bool> estop{false};

  ASSERT_TRUE(dev->Start(q, &estop, &err)) << err;

  auto st = dev->GetStatusSnapshot();
  EXPECT_EQ(st.device_id, "uuv-1");
  EXPECT_TRUE(st.conn_state == my_data::DeviceConnState::Online);

  dev->Stop();
  q.Shutdown(); // 测试中显式模拟“Edge 全局 shutdown 队列”
  dev->Join();
}

TEST(MyDevice_UUVDevice, ExecuteOneTask_BusyThenIdle) {
  auto dev = MyDevice::GetInstance().Create("uuv");
  ASSERT_TRUE(dev != nullptr);

  std::string err;
  nlohmann::json cfg = {
      {"device_id", "uuv-2"},
      {"device_name", "UUV-TWO"},
      {"control", nlohmann::json{{"simulate_latency_ms", 50}}} // 拉长一点便于观察 Busy
  };
  ASSERT_TRUE(dev->Init(cfg, &err)) << err;

  my_control::TaskQueue q("queue-uuv-2");
  std::atomic<bool> estop{false};
  ASSERT_TRUE(dev->Start(q, &estop, &err)) << err;

  my_data::Task t;
  t.task_id = "task-1";
  t.device_id = "uuv-2";
  t.capability = "navigate";
  t.action = "set";
  q.Push(t);

  // 1) 先观察到 Busy + running_task_id
  bool saw_busy = false;
  for (int i = 0; i < 100; ++i) {
    auto st = dev->GetStatusSnapshot();
    if (st.work_state == my_data::DeviceWorkState::Busy) {
      saw_busy = true;
      EXPECT_EQ(st.running_task_id, "task-1");
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  EXPECT_TRUE(saw_busy);

  // 2) 再观察到 Idle/Faulted 且 running_task_id 清空，并且 last_task_at_ms 被写入
  bool finished = false;
  for (int i = 0; i < 200; ++i) {
    auto st = dev->GetStatusSnapshot();
    if (st.last_task_at_ms > 0 &&
        (st.work_state == my_data::DeviceWorkState::Idle ||
         st.work_state == my_data::DeviceWorkState::Faulted)) {
      finished = true;
      EXPECT_TRUE(st.running_task_id.empty());
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  EXPECT_TRUE(finished);

  dev->Stop();
  q.Shutdown();
  dev->Join();
}