#pragma once

#include <optional>
#include <string>

#include "MyDB.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_db::demo {

class TaskRepository {
public:
  static TaskRepository& GetInstance();

  bool UpsertTask(const my_data::Task& task, std::string* err);
  bool UpsertResult(const my_data::TaskId& task_id, const my_data::TaskResult& r, std::string* err);

  bool ExistsTask(const my_data::TaskId& task_id, bool* exists, std::string* err);

  // 为 CRUD 测试提供：读取存储的 JSON 字符串（当前实现存的是 toString()）
  bool GetTaskJson(const my_data::TaskId& task_id, std::string* out_task_json, std::string* err);
  bool GetResultJson(const my_data::TaskId& task_id, std::string* out_result_json, std::string* err);

  bool UpdateTaskState(const my_data::TaskId& task_id, my_data::TaskState state, std::string* err);

  // 删除任务（results 通过外键 ON DELETE CASCADE 自动删除）
  bool DeleteTask(const my_data::TaskId& task_id, std::string* err);

private:
  TaskRepository() = default;
};

} // namespace my_db::demo