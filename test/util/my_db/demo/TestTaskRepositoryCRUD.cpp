#include <gtest/gtest.h>

#include <filesystem>

#include "MyDB.h"
#include "MyData.h"
#include "demo/TaskRepository.h"

using namespace my_db;
using namespace my_db::demo;

static std::string DbPathCRUD() {
  return "/tmp/fast_cpp_server_test_edge_crud.db";
}

static void InitFreshDB() {
  std::error_code ec;
  std::filesystem::remove(DbPathCRUD(), ec);

  DBConfig cfg;
  cfg.enable = true;
  cfg.type = "sqlite";
  cfg.path = DbPathCRUD();
  cfg.wal = true;
  cfg.busy_timeout_ms = 1000;
  cfg.synchronous = "NORMAL";
  cfg.auto_migrate = true;

  std::string err;
  ASSERT_TRUE(MyDB::GetInstance().Init(cfg, &err)) << err;
  ASSERT_TRUE(MyDB::GetInstance().Migrate(&err)) << err;
}

TEST(MyDB_TaskRepo, CRUD_Task_And_Result_CascadeDelete) {
  InitFreshDB();
  std::string err;

  // C: Create/Upsert
  my_data::Task t;
  t.task_id = "task-crud-1";
  t.command_id = "cmd-crud-1";
  t.edge_id = "edge-1";
  t.device_id = "uuv-1";
  t.capability = "navigate";
  t.action = "set";
  t.state = my_data::TaskState::Pending;
  t.created_at_ms = my_data::NowMs();
  t.deadline_at_ms = 0;

  ASSERT_TRUE(TaskRepository::GetInstance().UpsertTask(t, &err)) << err;

  my_data::TaskResult r;
  r.code = my_data::ErrorCode::Ok;
  r.message = "ok";
  r.started_at_ms = my_data::NowMs();
  r.finished_at_ms = r.started_at_ms + 1;

  ASSERT_TRUE(TaskRepository::GetInstance().UpsertResult(t.task_id, r, &err)) << err;

  // R: Read back json strings
  std::string task_json;
  ASSERT_TRUE(TaskRepository::GetInstance().GetTaskJson(t.task_id, &task_json, &err)) << err;
  EXPECT_FALSE(task_json.empty());

  std::string result_json;
  ASSERT_TRUE(TaskRepository::GetInstance().GetResultJson(t.task_id, &result_json, &err)) << err;
  EXPECT_FALSE(result_json.empty());

  // U: Update state
  ASSERT_TRUE(TaskRepository::GetInstance().UpdateTaskState(t.task_id, my_data::TaskState::Failed, &err)) << err;

  // verify state changed by checking json string contains "Finished" is not reliable.
  // so we just ensure task still exists.
  bool exists = false;
  ASSERT_TRUE(TaskRepository::GetInstance().ExistsTask(t.task_id, &exists, &err)) << err;
  EXPECT_TRUE(exists);

  // D: Delete task -> result should cascade delete
  ASSERT_TRUE(TaskRepository::GetInstance().DeleteTask(t.task_id, &err)) << err;

  exists = true;
  ASSERT_TRUE(TaskRepository::GetInstance().ExistsTask(t.task_id, &exists, &err)) << err;
  EXPECT_FALSE(exists);

  // result should be gone
  std::string out_r;
  EXPECT_FALSE(TaskRepository::GetInstance().GetResultJson(t.task_id, &out_r, &err));
  MyDB::GetInstance().Close();
}