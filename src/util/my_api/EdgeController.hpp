#pragma once
#ifndef EDGE_CONTROLLER_HPP
#define EDGE_CONTROLLER_HPP

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/core/Types.hpp"
// #include "oatpp/macro/codegen.hpp"
// #include "oatpp/macro/component.hpp"
// #include "oatpp/Types.hpp"

// 业务头文件
#include "MyEdgeManager.h"
#include "MyLog.h"

namespace my_api {

#include OATPP_CODEGEN_BEGIN(DTO)

class EdgeStatusDto : public oatpp::DTO {
    DTO_INIT(EdgeStatusDto, DTO)
    DTO_FIELD(String, name);
    DTO_FIELD(String, ip);
    DTO_FIELD(Boolean, online);
    DTO_FIELD(String, biz_status);
    DTO_FIELD(String, thread_id);
};

#include OATPP_CODEGEN_END(DTO)

#include OATPP_CODEGEN_BEGIN(ApiController)

class EdgeController : public oatpp::web::server::api::ApiController {
public:
    // --- 1. 修改构造函数：显式接收 ObjectMapper ---
    EdgeController(const std::shared_ptr<ObjectMapper>& objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper) 
    {}

    // --- 2. 修改工厂方法：显式接收并传递 ObjectMapper ---
    static std::shared_ptr<EdgeController> createShared(const std::shared_ptr<ObjectMapper>& objectMapper) {
        return std::make_shared<EdgeController>(objectMapper);
    }

    ENDPOINT("GET", "/v1/edges/status", getEdgesStatus) {
        MYLOG_INFO("[API] 收到请求: GET /v1/edges/status");
        auto status_list = edge_manager::MyEdgeManager::GetInstance().ShowEdgesStatus();
        
        auto result = oatpp::Vector<oatpp::Object<EdgeStatusDto>>::createShared();
        for (const auto& item : status_list) {
            auto dto = EdgeStatusDto::createShared();
            dto->name = item.value("name", "").c_str();
            dto->ip = item.value("ip", "").c_str();
            dto->online = item.value("online", false);
            dto->biz_status = item.value("biz_status", "").c_str();
            dto->thread_id = item.value("thread_id", "").c_str();
            result->push_back(dto);
        }
        return createDtoResponse(Status::CODE_200, result);
    }

    ENDPOINT("POST", "/v1/edges/{name}/start", startDevice, PATH(String, name)) {
        // 安全转换 String
        std::string deviceName = (name && name->size() > 0) ? name->c_str() : "";
        MYLOG_INFO("[API] 启动设备: {}", deviceName);
        edge_manager::MyEdgeManager::GetInstance().StartDevice(deviceName);
        return createResponse(Status::CODE_200, "Device start command issued.");
    }

    ENDPOINT("POST", "/v1/edges/{name}/stop", stopDevice, PATH(String, name)) {
        std::string deviceName = (name && name->size() > 0) ? name->c_str() : "";
        MYLOG_INFO("[API] 停止设备: {}", deviceName);
        edge_manager::MyEdgeManager::GetInstance().StopDevice(deviceName);
        return createResponse(Status::CODE_200, "Device stop command issued.");
    }
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api

#endif