#include "MyEdge.h"

#include "demo/UUVEdge.h"

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

  MYLOG_WARN("[MyEdge] Create: unknown type={}, return nullptr", type);
  return nullptr;
}

} // namespace my_edge