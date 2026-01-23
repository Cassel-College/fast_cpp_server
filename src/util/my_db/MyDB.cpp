#include "MyDB.h"

#include <filesystem>
#include <sstream>

#include "sqlite3.h"

namespace my_db {

DBConfig DBConfig::FromJson(const nlohmann::json& j) {
  DBConfig c;
  if (!j.is_object()) return c;

  c.enable = j.value("enable", c.enable);
  c.type = j.value("type", c.type);
  c.path = j.value("path", c.path);

  c.busy_timeout_ms = j.value("busy_timeout_ms", c.busy_timeout_ms);
  c.wal = j.value("wal", c.wal);
  c.synchronous = j.value("synchronous", c.synchronous);

  c.auto_migrate = j.value("auto_migrate", c.auto_migrate);

  c.status_snapshot_enable = j.value("status_snapshot_enable", c.status_snapshot_enable);
  c.status_snapshot_interval_ms = j.value("status_snapshot_interval_ms", c.status_snapshot_interval_ms);

  return c;
}

MyDB& MyDB::GetInstance() {
  static MyDB inst;
  return inst;
}

MyDB::~MyDB() {
  Close();
}

bool MyDB::IsInitialized() const {
  std::lock_guard<std::mutex> lk(mu_);
  return initialized_;
}

std::string MyDB::Path() const {
  std::lock_guard<std::mutex> lk(mu_);
  return cfg_.path;
}

bool MyDB::EnsureParentDirExists(const std::string& path, std::string* err) {
  try {
    std::filesystem::path p(path);
    auto parent = p.parent_path();
    if (parent.empty()) return true;

    if (!std::filesystem::exists(parent)) {
      std::filesystem::create_directories(parent);
      MYLOG_INFO("[MyDB] Created db parent dir: {}", parent.string());
    }
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    return false;
  }
}

bool MyDB::Init(const DBConfig& cfg, std::string* err) {
  std::lock_guard<std::mutex> lk(mu_);

  if (initialized_) {
    MYLOG_WARN("[MyDB] Init ignored: already initialized. path={}", cfg_.path);
    return true;
  }

  cfg_ = cfg;

  if (!cfg_.enable) {
    initialized_ = false;
    MYLOG_WARN("[MyDB] Init skipped: db.enable=false");
    return true;
  }

  if (cfg_.type != "sqlite") {
    std::string e = "unsupported db.type=" + cfg_.type + " (only sqlite supported)";
    if (err) *err = e;
    MYLOG_ERROR("[MyDB] Init failed: {}", e);
    return false;
  }

  std::string dir_err;
  if (!EnsureParentDirExists(cfg_.path, &dir_err)) {
    if (err) *err = dir_err;
    MYLOG_ERROR("[MyDB] Init failed: cannot create parent dir, err={}", dir_err);
    return false;
  }

  MYLOG_INFO("[MyDB] Init begin: path={}, busy_timeout_ms={}, wal={}, synchronous={}",
             cfg_.path, cfg_.busy_timeout_ms, cfg_.wal, cfg_.synchronous);

  int rc = sqlite3_open(cfg_.path.c_str(), &db_);
  if (rc != SQLITE_OK || !db_) {
    std::string e = std::string("sqlite3_open failed: ") + (db_ ? sqlite3_errmsg(db_) : "db is null");
    if (err) *err = e;
    MYLOG_ERROR("[MyDB] Init failed: {}", e);
    if (db_) {
      sqlite3_close(db_);
      db_ = nullptr;
    }
    return false;
  }

  sqlite3_busy_timeout(db_, cfg_.busy_timeout_ms);

  std::string pragma_err;
  if (!ApplyPragmasLocked(&pragma_err)) {
    if (err) *err = pragma_err;
    MYLOG_ERROR("[MyDB] Init failed: ApplyPragmasLocked err={}", pragma_err);
    sqlite3_close(db_);
    db_ = nullptr;
    return false;
  }

  initialized_ = true;
  MYLOG_INFO("[MyDB] Init success: path={}", cfg_.path);
  return true;
}

bool MyDB::ApplyPragmasLocked(std::string* err) {
  if (cfg_.wal) {
    if (!ExecLocked("PRAGMA journal_mode=WAL;", err)) return false;
  } else {
    if (!ExecLocked("PRAGMA journal_mode=DELETE;", err)) return false;
  }

  std::string sync = cfg_.synchronous;
  if (sync != "OFF" && sync != "NORMAL" && sync != "FULL") {
    MYLOG_WARN("[MyDB] Unknown synchronous={}, fallback NORMAL", sync);
    sync = "NORMAL";
  }
  if (!ExecLocked("PRAGMA synchronous=" + sync + ";", err)) return false;

  if (!ExecLocked("PRAGMA foreign_keys=ON;", err)) return false;

  return true;
}

bool MyDB::Exec(const std::string& sql, std::string* err) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!initialized_ || !db_) {
    std::string e = "db not initialized";
    if (err) *err = e;
    MYLOG_ERROR("[MyDB] Exec failed: {}", e);
    return false;
  }
  return ExecLocked(sql, err);
}

bool MyDB::ExecLocked(const std::string& sql, std::string* err) {
  char* errmsg = nullptr;
  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    std::string e = errmsg ? errmsg : sqlite3_errmsg(db_);
    if (errmsg) sqlite3_free(errmsg);
    if (err) *err = e;

    MYLOG_ERROR("[MyDB] Exec failed: rc={}, sql={}, err={}", rc, sql, e);
    return false;
  }

  MYLOG_DEBUG("[MyDB] Exec ok: {}", sql);
  return true;
}

bool MyDB::Transaction(const std::function<bool(std::string* err)>& fn, std::string* err) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!initialized_ || !db_) {
    std::string e = "db not initialized";
    if (err) *err = e;
    MYLOG_ERROR("[MyDB] Transaction failed: {}", e);
    return false;
  }
  return TransactionLocked(fn, err);
}

bool MyDB::TransactionLocked(const std::function<bool(std::string* err)>& fn, std::string* err) {
  std::string e1;
  if (!ExecLocked("BEGIN IMMEDIATE;", &e1)) {
    if (err) *err = e1;
    return false;
  }

  std::string fn_err;
  bool ok = false;
  try {
    ok = fn(&fn_err);
  } catch (const std::exception& e) {
    fn_err = e.what();
    ok = false;
  } catch (...) {
    fn_err = "unknown exception";
    ok = false;
  }

  if (ok) {
    std::string e2;
    if (!ExecLocked("COMMIT;", &e2)) {
      if (err) *err = e2;
      (void)ExecLocked("ROLLBACK;", nullptr);
      return false;
    }
    return true;
  } else {
    (void)ExecLocked("ROLLBACK;", nullptr);
    if (err) *err = fn_err.empty() ? "transaction aborted" : fn_err;
    return false;
  }
}

bool MyDB::Migrate(std::string* err) {
  std::lock_guard<std::mutex> lk(mu_);

  if (!cfg_.enable) {
    MYLOG_WARN("[MyDB] Migrate skipped: db.enable=false");
    return true;
  }
  if (!initialized_ || !db_) {
    std::string e = "db not initialized";
    if (err) *err = e;
    MYLOG_ERROR("[MyDB] Migrate failed: {}", e);
    return false;
  }

  MYLOG_INFO("[MyDB] Migrate begin");

  if (!ExecLocked("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER NOT NULL);", err)) return false;

  std::string tx_err;
  bool ok = TransactionLocked([&](std::string* txe) -> bool {
    if (!ExecLocked("INSERT INTO schema_version(version) SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM schema_version);", txe)) return false;

    if (!ExecLocked(R"SQL(
      CREATE TABLE IF NOT EXISTS tasks (
        task_id TEXT PRIMARY KEY,
        command_id TEXT,
        edge_id TEXT,
        device_id TEXT,
        capability TEXT,
        action TEXT,
        state INTEGER,
        created_at_ms INTEGER,
        deadline_at_ms INTEGER,
        task_json TEXT
      );
    )SQL", txe)) return false;

    if (!ExecLocked("CREATE INDEX IF NOT EXISTS idx_tasks_device_created ON tasks(device_id, created_at_ms);", txe)) return false;
    if (!ExecLocked("CREATE INDEX IF NOT EXISTS idx_tasks_command_id ON tasks(command_id);", txe)) return false;

    if (!ExecLocked(R"SQL(
      CREATE TABLE IF NOT EXISTS task_results (
        task_id TEXT PRIMARY KEY,
        code INTEGER,
        message TEXT,
        started_at_ms INTEGER,
        finished_at_ms INTEGER,
        result_json TEXT,
        FOREIGN KEY(task_id) REFERENCES tasks(task_id) ON DELETE CASCADE
      );
    )SQL", txe)) return false;

    if (!ExecLocked(R"SQL(
      CREATE TABLE IF NOT EXISTS device_status_snapshots (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        edge_id TEXT,
        device_id TEXT,
        ts_ms INTEGER,
        status_json TEXT
      );
    )SQL", txe)) return false;
    if (!ExecLocked("CREATE INDEX IF NOT EXISTS idx_dev_status_device_ts ON device_status_snapshots(device_id, ts_ms);", txe)) return false;

    if (!ExecLocked(R"SQL(
      CREATE TABLE IF NOT EXISTS edge_status_snapshots (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        edge_id TEXT,
        ts_ms INTEGER,
        status_json TEXT
      );
    )SQL", txe)) return false;
    if (!ExecLocked("CREATE INDEX IF NOT EXISTS idx_edge_status_edge_ts ON edge_status_snapshots(edge_id, ts_ms);", txe)) return false;

    return true;
  }, &tx_err);

  if (!ok) {
    if (err) *err = tx_err;
    MYLOG_ERROR("[MyDB] Migrate failed: {}", tx_err);
    return false;
  }

  MYLOG_INFO("[MyDB] Migrate success");
  return true;
}

void MyDB::Close() {
  std::lock_guard<std::mutex> lk(mu_);
  if (db_) {
    MYLOG_WARN("[MyDB] Close db: path={}", cfg_.path);
    sqlite3_close(db_);
    db_ = nullptr;
  }
  initialized_ = false;
}

} // namespace my_db