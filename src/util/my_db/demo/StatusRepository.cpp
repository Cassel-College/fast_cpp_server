#include "StatusRepository.h"

#include "sqlite3.h"

namespace my_db::demo {

StatusRepository& StatusRepository::GetInstance() {
  static StatusRepository inst;
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

bool StatusRepository::InsertDeviceSnapshot(const my_data::EdgeId& edge_id,
                                            const my_data::DeviceStatus& st,
                                            std::string* err) {
  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = R"SQL(
    INSERT INTO device_status_snapshots(edge_id, device_id, ts_ms, status_json)
    VALUES(?, ?, ?, ?);
  )SQL";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    MYLOG_ERROR("[StatusRepo] InsertDeviceSnapshot prepare failed: {}", err ? *err : "");
    return false;
  }

  std::string berr;
  bool ok = true;
  ok = ok && BindText(stmt, 1, edge_id, &berr);
  ok = ok && BindText(stmt, 2, st.device_id, &berr);
  ok = ok && BindInt64(stmt, 3, my_data::NowMs(), &berr);
  ok = ok && BindText(stmt, 4, st.toString(), &berr);

  if (!ok) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    MYLOG_ERROR("[StatusRepo] InsertDeviceSnapshot bind failed: {}", berr);
    return false;
  }

  std::string serr;
  ok = StepDone(stmt, h, &serr);
  sqlite3_finalize(stmt);

  if (!ok) {
    if (err) *err = serr;
    MYLOG_ERROR("[StatusRepo] InsertDeviceSnapshot step failed: {}", serr);
    return false;
  }

  MYLOG_DEBUG("[StatusRepo] InsertDeviceSnapshot ok: device_id={}", st.device_id);
  return true;
}

bool StatusRepository::InsertEdgeSnapshot(const my_data::EdgeStatus& st, std::string* err) {
  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = R"SQL(
    INSERT INTO edge_status_snapshots(edge_id, ts_ms, status_json)
    VALUES(?, ?, ?);
  )SQL";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    MYLOG_ERROR("[StatusRepo] InsertEdgeSnapshot prepare failed: {}", err ? *err : "");
    return false;
  }

  std::string berr;
  bool ok = true;
  ok = ok && BindText(stmt, 1, st.edge_id, &berr);
  ok = ok && BindInt64(stmt, 2, my_data::NowMs(), &berr);
  ok = ok && BindText(stmt, 3, st.toString(), &berr);

  if (!ok) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    MYLOG_ERROR("[StatusRepo] InsertEdgeSnapshot bind failed: {}", berr);
    return false;
  }

  std::string serr;
  ok = StepDone(stmt, h, &serr);
  sqlite3_finalize(stmt);

  if (!ok) {
    if (err) *err = serr;
    MYLOG_ERROR("[StatusRepo] InsertEdgeSnapshot step failed: {}", serr);
    return false;
  }

  MYLOG_DEBUG("[StatusRepo] InsertEdgeSnapshot ok: edge_id={}", st.edge_id);
  return true;
}

bool StatusRepository::CountEdgeSnapshots(const my_data::EdgeId& edge_id, std::int64_t* out_cnt, std::string* err) {
  if (out_cnt) *out_cnt = 0;

  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "SELECT COUNT(1) FROM edge_status_snapshots WHERE edge_id=?;";
  sqlite3_stmt* stmt = nullptr;

  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  if (!BindText(stmt, 1, edge_id, &berr)) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    if (out_cnt) *out_cnt = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return true;
  }

  if (err) *err = sqlite3_errmsg(h);
  sqlite3_finalize(stmt);
  return false;
}

bool StatusRepository::CountDeviceSnapshots(const my_data::EdgeId& edge_id, const my_data::DeviceId& device_id,
                                            std::int64_t* out_cnt, std::string* err) {
  if (out_cnt) *out_cnt = 0;

  auto& db = my_db::MyDB::GetInstance();
  std::lock_guard<std::mutex> lk(db.Mutex());

  sqlite3* h = db.UnsafeHandle();
  if (!h) {
    if (err) *err = "db not initialized";
    return false;
  }

  const char* sql = "SELECT COUNT(1) FROM device_status_snapshots WHERE edge_id=? AND device_id=?;";
  sqlite3_stmt* stmt = nullptr;

  int rc = sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK || !stmt) {
    if (err) *err = sqlite3_errmsg(h);
    return false;
  }

  std::string berr;
  if (!BindText(stmt, 1, edge_id, &berr) || !BindText(stmt, 2, device_id, &berr)) {
    sqlite3_finalize(stmt);
    if (err) *err = berr;
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    if (out_cnt) *out_cnt = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return true;
  }

  if (err) *err = sqlite3_errmsg(h);
  sqlite3_finalize(stmt);
  return false;
}

} // namespace my_db::demo