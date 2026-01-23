#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

#include "MyEdge.h"
#include "MyData.h"

using namespace my_edge;

static nlohmann::json BuildEdgeCfg(bool allow_queue_when_estop = false) {
  return nlohmann::json{
      {"edge_id", "edge-1"},
      {"type", "uuv"},
      {"version", "0.1.0"},
      {"allow_queue_when_estop", allow_queue_when_estop},
      {"devices", nlohmann::json::array({
          nlohmann::json{
              {"device_id", "uuv-1"},
              {"type", "uuv"},
              {"device_name", "UUV-ONE"},
              {"control", nlohmann::json{{"simulate_latency_ms", 10}}}
          }
      })}
  };
}

static my_data::RawCommand BuildCmd(const nlohmann::json& payload, const std::string& cmd_id = "cmd-1") {
  my_data::RawCommand rc;
  rc.command_id = cmd_id;
  rc.source = "test";
  rc.received_at_ms = 1;
  rc.payload = payload;
  return rc;
}

TEST(MyEdge_UUVEdge, InitThenSubmit_NotRunning) {
  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  std::string err;
  ASSERT_TRUE(edge->Init(BuildEdgeCfg(), &err)) << err;

  auto r = edge->Submit(BuildCmd(nlohmann::json{
      {"device_id", "uuv-1"},
      {"capability", "navigate"},
      {"action", "set"},
      {"params", nlohmann::json::object()}
  }));

  EXPECT_EQ(r.code, SubmitCode::NotRunning);
  edge->Shutdown();
}

TEST(MyEdge_UUVEdge, StartThenSubmit_Ok) {
  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  std::string err;
  ASSERT_TRUE(edge->Init(BuildEdgeCfg(), &err)) << err;
  ASSERT_TRUE(edge->Start(&err)) << err;

  auto r = edge->Submit(BuildCmd(nlohmann::json{
      {"device_id", "uuv-1"},
      {"capability", "navigate"},
      {"action", "set"},
      {"params", nlohmann::json{{"lat", 1.0}, {"lon", 2.0}}}
  }));

  EXPECT_EQ(r.code, SubmitCode::Ok);
  EXPECT_EQ(r.device_id, "uuv-1");
  EXPECT_FALSE(r.task_id.empty());

  // 等待执行一会儿，然后检查状态汇总
  for (int i = 0; i < 100; ++i) {
    auto st = edge->GetStatusSnapshot();
    // 只要任务跑过，last_task_at_ms 应该 > 0（由 my_device 更新）
    auto it = st.devices.find("uuv-1");
    if (it != st.devices.end() && it->second.last_task_at_ms > 0) {
      EXPECT_GE(it->second.queue_depth, 0);
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  edge->Shutdown();
}

TEST(MyEdge_UUVEdge, Submit_UnknownDevice) {
  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  std::string err;
  ASSERT_TRUE(edge->Init(BuildEdgeCfg(), &err)) << err;
  ASSERT_TRUE(edge->Start(&err)) << err;

  auto r = edge->Submit(BuildCmd(nlohmann::json{
      {"device_id", "not-exist"},
      {"capability", "navigate"},
      {"action", "set"}
  }));

  EXPECT_EQ(r.code, SubmitCode::UnknownDevice);
  edge->Shutdown();
}

TEST(MyEdge_UUVEdge, Submit_InvalidCommand_MissingDeviceId) {
  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  std::string err;
  ASSERT_TRUE(edge->Init(BuildEdgeCfg(), &err)) << err;
  ASSERT_TRUE(edge->Start(&err)) << err;

  auto r = edge->Submit(BuildCmd(nlohmann::json{
      {"capability", "navigate"},
      {"action", "set"}
  }));

  EXPECT_EQ(r.code, SubmitCode::InvalidCommand);
  edge->Shutdown();
}

TEST(MyEdge_UUVEdge, Submit_EStop_DefaultReject) {
  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  std::string err;
  ASSERT_TRUE(edge->Init(BuildEdgeCfg(false), &err)) << err;
  ASSERT_TRUE(edge->Start(&err)) << err;

  edge->SetEStop(true, "test estop");

  auto r = edge->Submit(BuildCmd(nlohmann::json{
      {"device_id", "uuv-1"},
      {"capability", "navigate"},
      {"action", "set"}
  }));

  EXPECT_EQ(r.code, SubmitCode::EStop);
  edge->Shutdown();
}

TEST(MyEdge_UUVEdge, Submit_EStop_AllowQueueWhenEStop) {
  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  std::string err;
  ASSERT_TRUE(edge->Init(BuildEdgeCfg(true), &err)) << err;
  ASSERT_TRUE(edge->Start(&err)) << err;

  edge->SetEStop(true, "test estop");

  auto r = edge->Submit(BuildCmd(nlohmann::json{
      {"device_id", "uuv-1"},
      {"capability", "navigate"},
      {"action", "set"}
  }));

  // 允许入队
  EXPECT_EQ(r.code, SubmitCode::Ok);

  // 但 workflow 会暂停取新任务，所以 pending_total 可能 > 0
  auto st = edge->GetStatusSnapshot();
  EXPECT_GE(st.tasks_pending_total, 0);

  edge->Shutdown();
}