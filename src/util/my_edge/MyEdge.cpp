#include "MyEdge.h"

#include "demo/UUVEdge.h"
#include "demo/TUNAEdge.h"
#include "demo/UAVEdge.h"
#include "demo/UNAEdge.h"
namespace my_edge {

MyEdge& MyEdge::GetInstance() {
  static MyEdge inst;
  return inst;
}

std::unique_ptr<IEdge> MyEdge::Create(const std::string& type) {
  MYLOG_INFO("[MyEdge] Create: type={}", type);

  if (type == "uuv" || type == "UUV") {
    return std::make_unique<my_edge::demo::UUVEdge>();
  }

  if (type == "uav" || type == "UAV") {
    return std::make_unique<my_edge::demo::UAVEdge>();
  }
  
  if (type == "tuna" || type == "TUNA") {
    return std::make_unique<my_edge::demo::TUNAEdge>();
  }

  MYLOG_WARN("[MyEdge] Create: unknown type={}, return nullptr", type);
  return nullptr;
}


std::unique_ptr<IEdge> MyEdge::Create(const std::string& type, const nlohmann::json& cfg, std::string* err) {
  MYLOG_INFO("[MyEdge] Create with cfg: type={}", type);

  if ("uuv" == type || "UUV" == type) {
    try{
      // 假设 UUVEdge 的构造函数接受 cfg 和 err 参数
      return std::make_unique<my_edge::demo::UUVEdge>(cfg, err);
    } catch (const std::exception& e) {
      if (err) {
        *err = std::string("Failed to create UUV edge: ") + e.what();
      }
      MYLOG_ERROR("[MyEdge] Create UUV edge failed: {}", e.what());
      return nullptr;
    }
  }

  if ("tuna" == type|| "TUNA" == type) {
    // 这里可以添加 TunaEdge 的创建逻辑
    try {
      // 假设 TunaEdge 的构造函数也接受 cfg 和 err 参数
      return std::make_unique<my_edge::demo::TUNAEdge>(cfg, err);
    } catch (const std::exception& e) {
      if (err) {
        *err = std::string("Failed to create TUNA edge: ") + e.what();
      }
      MYLOG_ERROR("[MyEdge] Create TUNA edge failed: {}", e.what());
      return nullptr;
    }
  }

  if ("uav" == type || "UAV" == type) {
    try {
      return std::make_unique<my_edge::demo::UAVEdge>(cfg, err);
    } catch (const std::exception& e) {
      if (err) {
        *err = std::string("Failed to create UAV edge: ") + e.what();
      }
      MYLOG_ERROR("[MyEdge] Create UAV edge failed: {}", e.what());
      return nullptr;
    }
  }

  if ("una" == type || "UNA" == type) {
    try {
      return std::make_unique<my_edge::demo::UNAEdge>(cfg, err);
    } catch (const std::exception& e) {
      if (err) {
        *err = std::string("Failed to create UNA edge: ") + e.what();
      }
      MYLOG_ERROR("[MyEdge] Create UNA edge failed: {}", e.what());
      return nullptr;
    }
  }

  MYLOG_WARN("[MyEdge] Create with cfg: unknown type={}, return nullptr", type);
  return nullptr;
}


} // namespace my_edge