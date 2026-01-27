#include "HeartBeatController.h"
#include "dto/HeartbeatDto.hpp"
#include "MyLog.h"

namespace my_api::heartbeat {

HeartBeatController::HeartBeatController(
    const std::shared_ptr<ObjectMapper>& objectMapper
)
    : BaseApiController(objectMapper) {}

std::shared_ptr<HeartBeatController>
HeartBeatController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper
) {
    return std::make_shared<HeartBeatController>(objectMapper);
}

// GET /v1/heartbeat
std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
HeartBeatController::getHeartbeat() {
    MYLOG_INFO("[API] Heartbeat GET");

    return ok("alive");
}

// POST /v1/heartbeat
std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
HeartBeatController::postHeartbeat(
    const oatpp::Object<my_api::dto::HeartbeatDto>& heartbeat
) {
    if (!heartbeat) {
        return badRequest("heartbeat body is empty");
    }

    MYLOG_INFO(
        "[API] Heartbeat POST from={}, timestamp={}, status={}",
        heartbeat->from ? heartbeat->from->c_str() : "null",
        heartbeat->timestamp ? *heartbeat->timestamp : 0,
        heartbeat->status ? heartbeat->status->c_str() : "null"
    );

    // 👉 后续你可以在这里：
    // - 更新设备在线状态
    // - 写数据库
    // - 更新心跳时间戳

    return ok("heartbeat received");
}

} // namespace my_api::heartbeat
