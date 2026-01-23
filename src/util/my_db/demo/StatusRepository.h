#pragma once

#include <cstdint>
#include <string>

#include "MyDB.h"
#include "MyData.h"
#include "MyLog.h"

namespace my_db::demo {

class StatusRepository {
public:
  static StatusRepository& GetInstance();

  bool InsertDeviceSnapshot(const my_data::EdgeId& edge_id,
                            const my_data::DeviceStatus& st,
                            std::string* err);

  bool InsertEdgeSnapshot(const my_data::EdgeStatus& st, std::string* err);

  // 为测试/观测提供：计数查询
  bool CountEdgeSnapshots(const my_data::EdgeId& edge_id, std::int64_t* out_cnt, std::string* err);
  bool CountDeviceSnapshots(const my_data::EdgeId& edge_id, const my_data::DeviceId& device_id,
                            std::int64_t* out_cnt, std::string* err);

private:
  StatusRepository() = default;
};

} // namespace my_db::demo