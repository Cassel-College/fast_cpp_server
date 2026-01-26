#pragma once

#include <memory>
#include <string>

#include "IEdge.h"
#include "MyLog.h"

namespace my_edge {

/**
 * @brief my_edge 工厂/门面
 */
class MyEdge {
public:
  static MyEdge& GetInstance();

  /**
   * @brief 按类型创建设备边缘运行时（demo）
   * @param type 例如 "uuv"
   */
  std::unique_ptr<IEdge> Create(const std::string& type);

  /**
   * @brief 按类型创建设备边缘运行时（demo）
   * @param type 例如 "uuv"
   * @param cfg 初始化配置
   */
  std::unique_ptr<IEdge> Create(const std::string& type, const nlohmann::json& cfg, std::string* err=nullptr);

private:
  MyEdge() = default;
};

} // namespace my_edge