#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "MyLog.h"

struct sqlite3; // forward declare

namespace my_db {

struct DBConfig {
  bool enable{true};
  std::string type{"sqlite"};
  std::string path{"/var/fast_cpp_server/data/edge.db"};

  int busy_timeout_ms{3000};
  bool wal{true};
  std::string synchronous{"NORMAL"};

  bool auto_migrate{true};

  bool status_snapshot_enable{true};
  int status_snapshot_interval_ms{5000};

  static DBConfig FromJson(const nlohmann::json& j);
};

class MyDB {
public:
  static MyDB& GetInstance();

  MyDB(const MyDB&) = delete;
  MyDB& operator=(const MyDB&) = delete;

  bool Init(const DBConfig& cfg, std::string* err);
  bool IsInitialized() const;
  bool Migrate(std::string* err);

  // 对外：会自动加锁
  bool Exec(const std::string& sql, std::string* err);

  // 对外：会自动加锁
  bool Transaction(const std::function<bool(std::string* err)>& fn, std::string* err);

  std::string Path() const;
  void Close();

  sqlite3* UnsafeHandle() const { return db_; }
  std::mutex& Mutex() { return mu_; }

private:
  MyDB() = default;
  ~MyDB();

  // 假设已持有 mu_
  bool ExecLocked(const std::string& sql, std::string* err);
  bool TransactionLocked(const std::function<bool(std::string* err)>& fn, std::string* err);

  bool ApplyPragmasLocked(std::string* err);
  bool EnsureParentDirExists(const std::string& path, std::string* err);

private:
  mutable std::mutex mu_;
  sqlite3* db_{nullptr};
  DBConfig cfg_{};
  bool initialized_{false};
};

} // namespace my_db