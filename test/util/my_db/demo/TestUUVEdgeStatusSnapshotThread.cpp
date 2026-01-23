#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <thread>

#include "MyDB.h"
#include "MyEdge.h"
#include "demo/StatusRepository.h"

using namespace my_db;
using namespace my_db::demo;
using namespace my_edge;

static std::string DbPathEdgeSnap() {
  return "/tmp/fast_cpp_server_test_edge_snap.db";
}

static nlohmann::json BuildEdgeCfgWithDB() {
  return nlohmann::json{
      {"edge_id", "edge-ss-1"},
      {"type", "uuv"},
      {"version", "0.1.0"},
      {"allow_queue_when_estop", false},
      {"db", nlohmann::json{
          {"enable", true},
          {"type", "sqlite"},
          {"path", DbPathEdgeSnap()},
          {"auto_migrate", true},
          {"wal", true},
          {"busy_timeout_ms", 1000},
          {"synchronous", "NORMAL"},
          {"status_snapshot_enable", true},
          {"status_snapshot_interval_ms", 200}
      }},
      {"devices", nlohmann::json::array({
          nlohmann::json{
              {"device_id", "uuv-1"},
              {"type", "uuv"},
              {"device_name", "UUV-ONE"},
              {"control", nlohmann::json{{"simulate_latency_ms", 1}}}
          }
      })}
  };
}

TEST(MyEdge_UUVEdge, StatusSnapshotThread_WritesToDB) {
  // fresh db
  std::error_code ec;
  std::filesystem::remove(DbPathEdgeSnap(), ec);

  // init db
  DBConfig dbc;
  dbc.enable = true;
  dbc.type = "sqlite";
  dbc.path = DbPathEdgeSnap();
  dbc.auto_migrate = true;
  dbc.wal = true;
  dbc.busy_timeout_ms = 1000;
  dbc.synchronous = "NORMAL";

  std::string err;
  ASSERT_TRUE(MyDB::GetInstance().Init(dbc, &err)) << err;
  ASSERT_TRUE(MyDB::GetInstance().Migrate(&err)) << err;

  auto edge = MyEdge::GetInstance().Create("uuv");
  ASSERT_TRUE(edge != nullptr);

  ASSERT_TRUE(edge->Init(BuildEdgeCfgWithDB(), &err)) << err;
  ASSERT_TRUE(edge->Start(&err)) << err;

  // wait snapshots written
  std::int64_t edge_cnt = 0;
  std::int64_t dev_cnt = 0;
  bool ok = false;
  for (int i = 0; i < 50; ++i) {
    std::string e1, e2;
    bool ok1 = StatusRepository::GetInstance().CountEdgeSnapshots("edge-ss-1", &edge_cnt, &e1);
    bool ok2 = StatusRepository::GetInstance().CountDeviceSnapshots("edge-ss-1", "uuv-1", &dev_cnt, &e2);
    if (ok1 && ok2 && edge_cnt > 0 && dev_cnt > 0) {
      ok = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  EXPECT_TRUE(ok);
  EXPECT_GT(edge_cnt, 0);
  EXPECT_GT(dev_cnt, 0);

  edge->Shutdown();
  MyDB::GetInstance().Close();
}