#include "TaskRepository.h"

#include <sstream>

#include "sqlite3.h"

namespace my_db::demo {

TaskRepository& TaskRepository::GetInstance() {
  static TaskRepository inst;
  return inst;
}

static bool BindText(sqlite3_stmt* stmt, int idx, const std::string& s, std::string* err) {
  if (sqlite3_bind_text(stmt, idx, s.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    if (err) *err = "sqlite3_bind_text failed";
    return false;
  }
  return true;
}

static bool BindInt64(sqlite3_stmt* stmt, int idx, std::int64_t v, std::string* err) {
  if (sqlite3_bind_int64(stmt, idx, static_cast<sqlite3_int64>(v)) != SQLITE_OK) {
    if (err) *err = "sqlite3_bind_int64 failed";
    return false;
  }
  return true;
}

static bool StepDone(sqlite3_stmt* stmt, sqlite3* db, std::string* err) {
  int rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
    if (err) *err = sqlite3_errmsg(db);
    return false;
  }
  return true;
}

bool TaskRepository::UpsertTask(const my_data::Task& task, std::string* err) {
  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = R"SQL(
    INSERT INTO tasks(task_id, command_id, edge_id, device_id, capability, action, state, created_at_ms, deadline_at_ms, task_json)
    VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ON CONFLICT(task_id) DO UPDATE SET
      command_id=excluded.command_id,
      edge_id=excluded.edge_id,
      device_id=excluded.device_id,
      capability=excluded.capability,
      action=excluded.action,
      state=excluded.state,
      created_at_ms=excluded.created_at_ms,
      deadline_at_ms=excluded.deadline_at_ms,
      task_json=excluded.task_json;
  )SQL";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    MYLOG_ERROR("[TaskRepo] UpsertTask prepare failed: err={}", err ? *err : "");
    return false;
  }

  std::string berr;
  bool ok = true;
  ok = ok && BindText(stmt, 1, task.task_id, &berr);
  ok = ok && BindText(stmt, 2, task.command_id, &berr);
  ok = ok && BindText(stmt, 3, task.edge_id, &berr);
  ok = ok && BindText(stmt, 4, task.device_id, &berr);
  ok = ok && BindText(stmt, 5, task.capability, &berr);
  ok = ok && BindText(stmt, 6, task.action, &berr);
  ok = ok && BindInt64(stmt, 7, static_cast<std::int64_t>(task.state), &berr);
  ok = ok && BindInt64(stmt, 8, task.created_at_ms, &berr);
  ok = ok && BindInt64(stmt, 9, task.deadline_at_ms, &berr);
  ok = ok && BindText(stmt, 10, task.toString(), &berr);

  if (!ok) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    MYLOG_ERROR("[TaskRepo] UpsertTask bind failed: {}", berr);
    return false;
  }

  std::string serr;
  ok = StepDone(stmt, h, &serr);
  sqlite3_finalize(stmt);

  if (!ok) {
    if (err) *err = serr;
    MYLOG_ERROR("[TaskRepo] UpsertTask step failed: {}", serr);
    return false;
  }

  MYLOG_INFO("[TaskRepo] UpsertTask ok: task_id={}, device_id={}, state={}",
             task.task_id, task.device_id, my_data::ToString(task.state));
  return true;
}

bool TaskRepository::UpsertResult(const my_data::TaskId& task_id, const my_data::TaskResult& r, std::string* err) {
  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = R"SQL(
    INSERT INTO task_results(task_id, code, message, started_at_ms, finished_at_ms, result_json)
    VALUES(?, ?, ?, ?, ?, ?)
    ON CONFLICT(task_id) DO UPDATE SET
      code=excluded.code,
      message=excluded.message,
      started_at_ms=excluded.started_at_ms,
      finished_at_ms=excluded.finished_at_ms,
      result_json=excluded.result_json;
  )SQL";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    MYLOG_ERROR("[TaskRepo] UpsertResult prepare failed: err={}", err ? *err : "");
    return false;
  }

  std::string berr;
  bool ok = true;
  ok = ok && BindText(stmt, 1, task_id, &berr);
  ok = ok && BindInt64(stmt, 2, static_cast<std::int64_t>(r.code), &berr);
  ok = ok && BindText(stmt, 3, r.message, &berr);
  ok = ok && BindInt64(stmt, 4, r.started_at_ms, &berr);
  ok = ok && BindInt64(stmt, 5, r.finished_at_ms, &berr);
  ok = ok && BindText(stmt, 6, r.toString(), &berr);

  if (!ok) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    MYLOG_ERROR("[TaskRepo] UpsertResult bind failed: {}", berr);
    return false;
  }

  std::string serr;
  ok = StepDone(stmt, h, &serr);
  sqlite3_finalize(stmt);

  if (!ok) {
    if (err) *err = serr;
    MYLOG_ERROR("[TaskRepo] UpsertResult step failed: {}", serr);
    return false;
  }

  MYLOG_INFO("[TaskRepo] UpsertResult ok: task_id={}, code={}", task_id, my_data::ToString(r.code));
  return true;
}

bool TaskRepository::ExistsTask(const my_data::TaskId& task_id, bool* exists, std::string* err) {
  if (exists) *exists = false;

  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "SELECT 1 FROM tasks WHERE task_id=? LIMIT 1;";
  sqlite3_stmt* stmt = nullptr;

  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  if (!BindText(stmt, 1, task_id, &berr)) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    if (exists) *exists = true;
  } else if (rc == SQLITE_DONE) {
    if (exists) *exists = false;
  } else {
    if (err) *err = sqlite3_errmsg(h);
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

bool TaskRepository::GetTaskJson(const my_data::TaskId& task_id, std::string* out_task_json, std::string* err) {
  if (out_task_json) out_task_json->clear();

  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "SELECT task_json FROM tasks WHERE task_id=? LIMIT 1;";
  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  if (!BindText(stmt, 1, task_id, &berr)) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char* txt = sqlite3_column_text(stmt, 0);
    if (out_task_json) *out_task_json = txt ? reinterpret_cast<const char*>(txt) : "";
    sqlite3_finalize(stmt);
    return true;
  }
  if (rc == SQLITE_DONE) {
    if (err) *err = "not found";
    sqlite3_finalize(stmt);
    return false;
  }

  if (err) *err = sqlite3_errmsg(h);
  sqlite3_finalize(stmt);
  return false;
}

bool TaskRepository::GetResultJson(const my_data::TaskId& task_id, std::string* out_result_json, std::string* err) {
  if (out_result_json) out_result_json->clear();

  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "SELECT result_json FROM task_results WHERE task_id=? LIMIT 1;";
  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  if (!BindText(stmt, 1, task_id, &berr)) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char* txt = sqlite3_column_text(stmt, 0);
    if (out_result_json) *out_result_json = txt ? reinterpret_cast<const char*>(txt) : "";
    sqlite3_finalize(stmt);
    return true;
  }
  if (rc == SQLITE_DONE) {
    if (err) *err = "not found";
    sqlite3_finalize(stmt);
    return false;
  }

  if (err) *err = sqlite3_errmsg(h);
  sqlite3_finalize(stmt);
  return false;
}

bool TaskRepository::UpdateTaskState(const my_data::TaskId& task_id, my_data::TaskState state, std::string* err) {
  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "UPDATE tasks SET state=? WHERE task_id=?;";
  sqlite3_stmt* stmt = nullptr;

  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  bool ok = true;
  ok = ok && BindInt64(stmt, 1, static_cast<std::int64_t>(state), &berr);
  ok = ok && BindText(stmt, 2, task_id, &berr);

  if (!ok) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  std::string serr;
  ok = StepDone(stmt, h, &serr);
  sqlite3_finalize(stmt);

  if (!ok) {
    if (err) *err = serr;
    return false;
  }

  MYLOG_INFO("[TaskRepo] UpdateTaskState ok: task_id={}, state={}", task_id, my_data::ToString(state));
  return true;
}

bool TaskRepository::DeleteTask(const my_data::TaskId& task_id, std::string* err) {
  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "DELETE FROM tasks WHERE task_id=?;";
  sqlite3_stmt* stmt = nullptr;

  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  if (!BindText(stmt, 1, task_id, &berr)) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  std::string serr;
  bool ok = StepDone(stmt, h, &serr);
  sqlite3_finalize(stmt);

  if (!ok) {
    if (err) *err = serr;
    return false;
  }

  MYLOG_WARN("[TaskRepo] DeleteTask ok: task_id={}", task_id);
  return true;
}

} // namespace my_db::demo