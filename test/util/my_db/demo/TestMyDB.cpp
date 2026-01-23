#include <gtest/gtest.h>

#include <filesystem>

#include "MyDB.h"
#include "MyData.h"
#include "demo/TaskRepository.h"
#include "demo/StatusRepository.h"

using namespace my_data;
using namespace my_db;
using namespace my_db::demo;

static std::string TestDbPath() {
  // return "/tmp/fast_cpp_server_test_edge.db";
  return "/home/cs/DockerRoot/fast_cpp_server/fast_cpp_server_test_edge.db";
}

TEST(MyDB_SQLite, InitMigrateAndBasicRepos) {
  // 清理旧 db
  std::error_code ec;
  std::filesystem::remove(TestDbPath(), ec);

  DBConfig cfg;
  cfg.enable = true;
  cfg.type = "sqlite";
  cfg.path = TestDbPath();
  cfg.wal = true;
  cfg.busy_timeout_ms = 1000;
  cfg.synchronous = "NORMAL";
  cfg.auto_migrate = true;

  std::string err;
  ASSERT_TRUE(MyDB::GetInstance().Init(cfg, &err)) << err;
  ASSERT_TRUE(MyDB::GetInstance().Migrate(&err)) << err;

  // 写 task + result
  my_data::Task t;
  t.task_id = "task-1";
  t.command_id = "cmd-1";
  t.edge_id = "edge-1";
  t.device_id = "uuv-1";
  t.capability = "navigate";
  t.action = "set";
  // t.state = my_data::TaskState::Finished;
  t.state = my_data::TaskState::Failed;
  t.created_at_ms = my_data::NowMs();

  ASSERT_TRUE(TaskRepository::GetInstance().UpsertTask(t, &err)) << err;

  my_data::TaskResult r;
  r.code = my_data::ErrorCode::Ok;
  r.message = "ok";
  r.started_at_ms = my_data::NowMs();
  r.finished_at_ms = r.started_at_ms + 5;

  ASSERT_TRUE(TaskRepository::GetInstance().UpsertResult(t.task_id, r, &err)) << err;

  bool exists = false;
  ASSERT_TRUE(TaskRepository::GetInstance().ExistsTask(t.task_id, &exists, &err)) << err;
  EXPECT_TRUE(exists);

  // 写状态快照
  my_data::DeviceStatus ds;
  ds.device_id = "uuv-1";
  ds.conn_state = my_data::DeviceConnState::Online;
  ds.work_state = my_data::DeviceWorkState::Idle;

  ASSERT_TRUE(StatusRepository::GetInstance().InsertDeviceSnapshot("edge-1", ds, &err)) << err;

  my_data::EdgeStatus es;
  es.edge_id = "edge-1";
  es.version = "0.1.0";
  es.boot_at_ms = my_data::NowMs();
  es.run_state = my_data::EdgeRunState::Running;

  ASSERT_TRUE(StatusRepository::GetInstance().InsertEdgeSnapshot(es, &err)) << err;

  MyDB::GetInstance().Close();
}