#include <gtest/gtest.h>

#include <filesystem>

#include "MyDB.h"
#include "MyData.h"
#include "demo/StatusRepository.h"

using namespace my_db;
using namespace my_db::demo;

static std::string DbPathStatus() {
  return "/tmp/fast_cpp_server_test_edge_status.db";
}

TEST(MyDB_StatusRepo, InsertSnapshots) {
  std::error_code ec;
  std::filesystem::remove(DbPathStatus(), ec);

  DBConfig cfg;
  cfg.enable = true;
  cfg.type = "sqlite";
  cfg.path = DbPathStatus();
  cfg.wal = true;
  cfg.busy_timeout_ms = 1000;
  cfg.synchronous = "NORMAL";

  std::string err;
  ASSERT_TRUE(MyDB::GetInstance().Init(cfg, &err)) << err;
  ASSERT_TRUE(MyDB::GetInstance().Migrate(&err)) << err;

  my_data::DeviceStatus ds;
  ds.device_id = "uuv-1";
  ds.conn_state = my_data::DeviceConnState::Online;
  ds.work_state = my_data::DeviceWorkState::Idle;

  for (int i = 0; i < 3; ++i) {
    ASSERT_TRUE(StatusRepository::GetInstance().InsertDeviceSnapshot("edge-1", ds, &err)) << err;
  }

  my_data::EdgeStatus es;
  es.edge_id = "edge-1";
  es.version = "0.1.0";
  es.boot_at_ms = my_data::NowMs();
  es.run_state = my_data::EdgeRunState::Running;

  for (int i = 0; i < 2; ++i) {
    ASSERT_TRUE(StatusRepository::GetInstance().InsertEdgeSnapshot(es, &err)) << err;
  }

  MyDB::GetInstance().Close();
}