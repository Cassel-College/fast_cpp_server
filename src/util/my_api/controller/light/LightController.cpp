#include "LightController.h"

#include <nlohmann/json.hpp>

#include "MyLog.h"
// my_light 模块头文件（由 CMakeLists 中 my_light PUBLIC include 目录提供）
#include "../../../my_light/SearchlightManager.h"

namespace my_api::light_api {

using namespace my_api::base;

// 每次 API 调用前允许的设备离线超时时间（毫秒）
static constexpr int kOfflineTimeoutMs = 3000;

// ============================================================================
// 构造 / 工厂
// ============================================================================

LightController::LightController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : BaseApiController(objectMapper) {}

std::shared_ptr<LightController> LightController::createShared(
    const std::shared_ptr<ObjectMapper>& objectMapper) {
    return std::make_shared<LightController>(objectMapper);
}

// ============================================================================
// 内部辅助：在线检测
// ============================================================================

namespace {

/**
 * @brief 检测探照灯设备是否在线
 *
 * 统一封装在线校验逻辑，所有控制接口在执行前均需调用此函数。
 * 若设备离线，调用方直接返回 503 错误响应。
 *
 * @param manager 探照灯管理器单例引用
 * @return true  设备在线，可继续执行控制命令
 * @return false 设备离线，不可发送命令
 */
bool isDeviceOnline(SearchlightControl::SearchlightManager& manager) {
    return manager.online();
}

} // namespace

// ============================================================================
// LED 控制接口
// ============================================================================

/**
 * @brief POST /v1/light/led/open — 开启 LED
 *
 * 先校验设备是否在线，在线后向探照灯发送 LED 开启命令。
 * 设备离线时立即返回 503，并说明原因。
 */
MyAPIResponsePtr LightController::openLed() {
    MYLOG_INFO("[API-Light] POST /v1/light/led/open — 请求开启 LED");

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 开启 LED 失败：探照灯设备当前离线，无法发送命令");
        return jsonError(503, "探照灯设备离线，无法执行开启 LED 操作");
    }

    // 发送开灯命令
    bool ok = manager.open_led();
    if (!ok) {
        MYLOG_ERROR("[API-Light] 开启 LED 失败：命令发送返回 false");
        return jsonError(500, "开启 LED 命令发送失败，请检查设备状态");
    }

    MYLOG_INFO("[API-Light] 开启 LED 命令发送成功");
    return jsonOk(nlohmann::json::object(), "LED 已开启");
}

/**
 * @brief POST /v1/light/led/close — 关闭 LED
 *
 * 先校验设备是否在线，在线后向探照灯发送 LED 关闭命令。
 * 设备离线时立即返回 503，并说明原因。
 */
MyAPIResponsePtr LightController::closeLed() {
    MYLOG_INFO("[API-Light] POST /v1/light/led/close — 请求关闭 LED");

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 关闭 LED 失败：探照灯设备当前离线，无法发送命令");
        return jsonError(503, "探照灯设备离线，无法执行关闭 LED 操作");
    }

    // 发送关灯命令
    bool ok = manager.close_led();
    if (!ok) {
        MYLOG_ERROR("[API-Light] 关闭 LED 失败：命令发送返回 false");
        return jsonError(500, "关闭 LED 命令发送失败，请检查设备状态");
    }

    MYLOG_INFO("[API-Light] 关闭 LED 命令发送成功");
    return jsonOk(nlohmann::json::object(), "LED 已关闭");
}

/**
 * @brief POST /v1/light/led/level — 设置 LED 亮度
 *
 * 校验 level 参数（0～100），再校验设备在线，最后下发亮度设置命令。
 * 参数超出范围返回 400，设备离线返回 503。
 */
MyAPIResponsePtr LightController::setLightLevel(
    const oatpp::Object<my_api::dto::LightLevelDto>& levelDto) {
    // 请求体非空校验
    if (!levelDto) {
        MYLOG_WARN("[API-Light] 设置亮度失败：请求体为空");
        return jsonError(400, "请求体不能为空，需提供 level 参数");
    }

    // level 参数合法性校验：0～100
    if (!levelDto->level) {
        MYLOG_WARN("[API-Light] 设置亮度失败：缺少 level 参数");
        return jsonError(400, "缺少参数 level，亮度级别范围为 0～100");
    }
    int level = *levelDto->level;
    if (level < 0 || level > 100) {
        MYLOG_WARN("[API-Light] 设置亮度失败：level={} 超出允许范围 0～100", level);
        return jsonError(400, "参数 level 超出范围，允许值为 0～100",
                         {{"received", level}, {"allowed_min", 0}, {"allowed_max", 100}});
    }

    MYLOG_INFO("[API-Light] POST /v1/light/led/level — 设置亮度，level={}", level);

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 设置亮度失败：探照灯设备当前离线，level={}", level);
        return jsonError(503, "探照灯设备离线，无法执行设置亮度操作");
    }

    // 发送亮度设置命令
    bool ok = manager.setting_light_level(level);
    if (!ok) {
        MYLOG_ERROR("[API-Light] 设置亮度失败：命令发送返回 false，level={}", level);
        return jsonError(500, "设置亮度命令发送失败", {{"level", level}});
    }

    MYLOG_INFO("[API-Light] 设置亮度成功，level={}", level);
    return jsonOk({{"level", level}}, "亮度设置成功");
}

// ============================================================================
// 频闪控制接口
// ============================================================================

/**
 * @brief POST /v1/light/flash/open — 开启频闪模式
 *
 * 先校验设备是否在线，在线后向探照灯发送频闪开启命令。
 * 设备离线时立即返回 503，并说明原因。
 */
MyAPIResponsePtr LightController::openFlash() {
    MYLOG_INFO("[API-Light] POST /v1/light/flash/open — 请求开启频闪");

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 开启频闪失败：探照灯设备当前离线，无法发送命令");
        return jsonError(503, "探照灯设备离线，无法执行开启频闪操作");
    }

    // 发送开启频闪命令
    bool ok = manager.open_flash();
    if (!ok) {
        MYLOG_ERROR("[API-Light] 开启频闪失败：命令发送返回 false");
        return jsonError(500, "开启频闪命令发送失败，请检查设备状态");
    }

    MYLOG_INFO("[API-Light] 开启频闪命令发送成功");
    return jsonOk(nlohmann::json::object(), "频闪模式已开启");
}

/**
 * @brief POST /v1/light/flash/close — 关闭频闪模式
 *
 * 先校验设备是否在线，在线后向探照灯发送频闪关闭命令。
 * 设备离线时立即返回 503，并说明原因。
 */
MyAPIResponsePtr LightController::closeFlash() {
    MYLOG_INFO("[API-Light] POST /v1/light/flash/close — 请求关闭频闪");

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 关闭频闪失败：探照灯设备当前离线，无法发送命令");
        return jsonError(503, "探照灯设备离线，无法执行关闭频闪操作");
    }

    // 发送关闭频闪命令
    bool ok = manager.close_flash();
    if (!ok) {
        MYLOG_ERROR("[API-Light] 关闭频闪失败：命令发送返回 false");
        return jsonError(500, "关闭频闪命令发送失败，请检查设备状态");
    }

    MYLOG_INFO("[API-Light] 关闭频闪命令发送成功");
    return jsonOk(nlohmann::json::object(), "频闪模式已关闭");
}

/**
 * @brief POST /v1/light/flash/setting — 设置频闪频率
 *
 * 校验 hz 参数（1～50），再校验设备在线，最后下发频率设置命令。
 * 参数超出范围返回 400，设备离线返回 503。
 */
MyAPIResponsePtr LightController::setFlash(
    const oatpp::Object<my_api::dto::LightFlashHzDto>& flashDto) {
    // 请求体非空校验
    if (!flashDto) {
        MYLOG_WARN("[API-Light] 设置频闪频率失败：请求体为空");
        return jsonError(400, "请求体不能为空，需提供 hz 参数");
    }

    // hz 参数合法性校验：1～50
    if (!flashDto->hz) {
        MYLOG_WARN("[API-Light] 设置频闪频率失败：缺少 hz 参数");
        return jsonError(400, "缺少参数 hz，频闪频率范围为 1～50 Hz");
    }
    int hz = *flashDto->hz;
    if (hz < 1 || hz > 50) {
        MYLOG_WARN("[API-Light] 设置频闪频率失败：hz={} 超出允许范围 1～50", hz);
        return jsonError(400, "参数 hz 超出范围，允许值为 1～50 Hz",
                         {{"received", hz}, {"allowed_min", 1}, {"allowed_max", 50}});
    }

    MYLOG_INFO("[API-Light] POST /v1/light/flash/setting — 设置频闪频率，hz={}", hz);

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 设置频闪频率失败：探照灯设备当前离线，hz={}", hz);
        return jsonError(503, "探照灯设备离线，无法执行设置频闪频率操作");
    }

    // 发送频率设置命令
    bool ok = manager.setting_flash(hz);
    if (!ok) {
        MYLOG_ERROR("[API-Light] 设置频闪频率失败：命令发送返回 false，hz={}", hz);
        return jsonError(500, "设置频闪频率命令发送失败", {{"hz", hz}});
    }

    MYLOG_INFO("[API-Light] 设置频闪频率成功，hz={}", hz);
    return jsonOk({{"hz", hz}}, "频闪频率设置成功");
}

// ============================================================================
// 云台控制接口
// ============================================================================

/**
 * @brief POST /v1/light/servo/home — 云台复位回中
 *
 * 先校验设备是否在线，在线后向探照灯发送云台复位命令。
 * 设备离线时立即返回 503，并说明原因。
 */
MyAPIResponsePtr LightController::servoHome() {
    MYLOG_INFO("[API-Light] POST /v1/light/servo/home — 请求云台复位");

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 云台复位失败：探照灯设备当前离线，无法发送命令");
        return jsonError(503, "探照灯设备离线，无法执行云台复位操作");
    }

    // 发送云台复位命令
    bool ok = manager.home();
    if (!ok) {
        MYLOG_ERROR("[API-Light] 云台复位失败：命令发送返回 false");
        return jsonError(500, "云台复位命令发送失败，请检查设备状态");
    }

    MYLOG_INFO("[API-Light] 云台复位命令发送成功");
    return jsonOk(nlohmann::json::object(), "云台已发送复位命令");
}

/**
 * @brief POST /v1/light/servo/move_to — 云台移动到绝对角度
 *
 * 校验 x(-180～180) 和 y(-180～180) 参数范围，再校验设备在线，最后下发移动命令。
 * 参数超出范围返回 400，设备离线返回 503。
 */
MyAPIResponsePtr LightController::servoMoveTo(
    const oatpp::Object<my_api::dto::LightMoveToDto>& moveDto) {
    // 请求体非空校验
    if (!moveDto) {
        MYLOG_WARN("[API-Light] 云台移动失败：请求体为空");
        return jsonError(400, "请求体不能为空，需提供 x 和 y 参数");
    }

    // x 参数合法性校验：-180～180
    if (!moveDto->x) {
        MYLOG_WARN("[API-Light] 云台移动失败：缺少 x 参数");
        return jsonError(400, "缺少参数 x，水平角度范围为 -180～180");
    }
    int x = *moveDto->x;
    if (x < -180 || x > 180) {
        MYLOG_WARN("[API-Light] 云台移动失败：x={} 超出允许范围 -180～180", x);
        return jsonError(400, "参数 x（水平角度）超出范围，允许值为 -180～180",
                         {{"received_x", x}, {"allowed_min", -180}, {"allowed_max", 180}});
    }

    // y 参数合法性校验：-180～180
    if (!moveDto->y) {
        MYLOG_WARN("[API-Light] 云台移动失败：缺少 y 参数");
        return jsonError(400, "缺少参数 y，垂直角度范围为 -180～180");
    }
    int y = *moveDto->y;
    if (y < -180 || y > 180) {
        MYLOG_WARN("[API-Light] 云台移动失败：y={} 超出允许范围 -180～180", y);
        return jsonError(400, "参数 y（垂直角度）超出范围，允许值为 -180～180",
                         {{"received_y", y}, {"allowed_min", -180}, {"allowed_max", 180}});
    }

    MYLOG_INFO("[API-Light] POST /v1/light/servo/move_to — 云台移动，x={}, y={}", x, y);

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 云台移动失败：探照灯设备当前离线，x={}, y={}", x, y);
        return jsonError(503, "探照灯设备离线，无法执行云台移动操作");
    }

    // 发送云台移动命令
    bool ok = manager.move_to(x, y);
    if (!ok) {
        MYLOG_ERROR("[API-Light] 云台移动失败：命令发送返回 false，x={}, y={}", x, y);
        return jsonError(500, "云台移动命令发送失败", {{"x", x}, {"y", y}});
    }

    MYLOG_INFO("[API-Light] 云台移动命令发送成功，x={}, y={}", x, y);
    return jsonOk({{"x", x}, {"y", y}}, "云台移动命令已发送");
}

/**
 * @brief POST /v1/light/servo/move_dir — 云台按方向步进移动
 *
 * 根据 direction（up/down/left/right）计算目标角度偏移（步长 5°），
 * 从设备当前状态快照中读取当前角度，叠加步长后调用 move_to。
 * direction 非法返回 400，设备离线返回 503，角度越界时截断到边界值。
 */
MyAPIResponsePtr LightController::servMoveDir(
    const oatpp::Object<my_api::dto::LightDirectionDto>& dirDto) {
    // 请求体非空校验
    if (!dirDto) {
        MYLOG_WARN("[API-Light] 云台方向移动失败：请求体为空");
        return jsonError(400, "请求体不能为空，需提供 direction 参数");
    }

    // direction 参数合法性校验
    if (!dirDto->direction) {
        MYLOG_WARN("[API-Light] 云台方向移动失败：缺少 direction 参数");
        return jsonError(400, "缺少参数 direction，有效值为 up / down / left / right");
    }
    std::string direction = std::string(dirDto->direction->c_str());
    if (direction != "up" && direction != "down" &&
        direction != "left" && direction != "right") {
        MYLOG_WARN("[API-Light] 云台方向移动失败：direction='{}' 不是合法方向", direction);
        return jsonError(400,
                         "参数 direction 非法，有效值为 up / down / left / right",
                         {{"received", direction}});
    }

    MYLOG_INFO("[API-Light] POST /v1/light/servo/move_dir — 方向移动，direction={}", direction);

    // 检查设备是否在线，离线直接拒绝
    auto& manager = SearchlightControl::SearchlightManager::getInstance();
    if (!isDeviceOnline(manager)) {
        MYLOG_WARN("[API-Light] 云台方向移动失败：探照灯设备当前离线，direction={}", direction);
        return jsonError(503, "探照灯设备离线，无法执行云台方向移动操作");
    }

    // 通过 get_status 读取当前角度，叠加固定步长（5°）后下发绝对移动命令
    constexpr int kStep = 5; // 每次移动固定步长（度）

    int cur_x = 0;
    int cur_y = 0;
    {
        std::string status_str;
        manager.get_status(status_str);
        try {
            // toStatusString() 返回的是合法 JSON 字符串，直接解析
            auto j = nlohmann::json::parse(status_str);
            cur_x  = j.value("servo_x_degree", 0);
            cur_y  = j.value("servo_y_degree", 0);
            MYLOG_DEBUG("[API-Light] 当前云台角度：x={}, y={}", cur_x, cur_y);
        } catch (const std::exception& e) {
            // 解析失败时以 (0,0) 为当前位置，记录警告但不中止操作
            MYLOG_WARN("[API-Light] 解析当前云台角度失败，将以 (0,0) 为基准移动：{}", e.what());
        }
    }

    // 根据方向计算新角度，超出 ±180 范围时截断到边界值
    int new_x = cur_x;
    int new_y = cur_y;
    if (direction == "up") {
        new_y = std::min(180, cur_y + kStep);
    } else if (direction == "down") {
        new_y = std::max(-180, cur_y - kStep);
    } else if (direction == "right") {
        new_x = std::min(180, cur_x + kStep);
    } else { // left
        new_x = std::max(-180, cur_x - kStep);
    }

    MYLOG_INFO("[API-Light] 方向移动计算：({},{}) -> ({},{})，direction={}",
               cur_x, cur_y, new_x, new_y, direction);

    // 发送云台绝对移动命令
    bool ok = manager.move_to(new_x, new_y);
    if (!ok) {
        MYLOG_ERROR("[API-Light] 方向移动失败：命令发送返回 false，direction={}, 目标=({},{})",
                    direction, new_x, new_y);
        return jsonError(500, "云台方向移动命令发送失败",
                         {{"direction", direction}, {"target_x", new_x}, {"target_y", new_y}});
    }

    MYLOG_INFO("[API-Light] 云台方向移动命令发送成功，direction={}, 目标=({},{})",
               direction, new_x, new_y);
    return jsonOk(
        {{"direction", direction}, {"target_x", new_x}, {"target_y", new_y}},
        "云台方向移动命令已发送");
}

// ============================================================================
// 状态查询接口
// ============================================================================

/**
 * @brief GET /v1/light/get_status — 查看探照灯结构化状态
 *
 * 解析底层状态快照，以语义化字段返回：
 *   - online        : 设备是否在线（bool）
 *   - led_on        : LED 是否开启（bool）
 *   - flash_enabled : 频闪是否开启（bool）
 *   - brightness    : 当前亮度 0～100（int，-1 表示未知）
 *   - flash_hz      : 当前频闪频率 Hz（int，-1 表示未知）
 *   - servo_x       : 云台水平角度（int）
 *   - servo_y       : 云台垂直角度（int）
 * 不需要设备在线也可查询。
 */
MyAPIResponsePtr LightController::getStatus() {
    MYLOG_INFO("[API-Light] GET /v1/light/get_status — 查询探照灯结构化状态");

    auto& manager = SearchlightControl::SearchlightManager::getInstance();

    // 通过 get_status 获取底层 JSON 快照后解析为结构化字段
    std::string status_str;
    manager.get_status(status_str);

    nlohmann::json raw;
    try {
        raw = nlohmann::json::parse(status_str);
    } catch (const std::exception& e) {
        MYLOG_WARN("[API-Light] getStatus：状态 JSON 解析失败：{}", e.what());
        return jsonError(500, "探照灯状态解析失败", {{"detail", e.what()}});
    }

    // 将原始字符串字段转换为语义化布尔 / 整数字段
    bool online         = (raw.value("device", "")  == "在线");
    bool led_on         = (raw.value("led",    "")  == "开启");
    bool flash_enabled  = (raw.value("flash",  "")  == "开启");
    int  brightness     = raw.value("brightness",     -1);
    int  flash_hz       = raw.value("flash_hz",       -1);
    int  servo_x        = raw.value("servo_x_degree",  0);
    int  servo_y        = raw.value("servo_y_degree",  0);

    nlohmann::json data = {
        {"online",        online},
        {"led_on",        led_on},
        {"flash_enabled", flash_enabled},
        {"brightness",    brightness},
        {"flash_hz",      flash_hz},
        {"servo_x",       servo_x},
        {"servo_y",       servo_y}
    };

    MYLOG_INFO("[API-Light] getStatus 成功：online={}, led={}, flash={}, brightness={}, hz={}, x={}, y={}",
               online, led_on, flash_enabled, brightness, flash_hz, servo_x, servo_y);
    return jsonOk(data, "探照灯状态获取成功");
}

/**
 * @brief GET /v1/light/status — 查看探照灯当前状态
 *
 * 返回探照灯设备状态快照，包括在线状态、LED 状态、频闪状态、亮度、云台角度等。
 * 不需要设备在线也可查询（可用于诊断离线原因）。
 */
MyAPIResponsePtr LightController::getLightStatus() {
    MYLOG_INFO("[API-Light] GET /v1/light/status — 查询探照灯状态");

    auto& manager = SearchlightControl::SearchlightManager::getInstance();

    // 获取设备状态字符串（JSON 格式）
    std::string status_str;
    manager.get_status(status_str);

    // 将 JSON 字符串嵌入统一响应格式
    nlohmann::json data;
    try {
        data = nlohmann::json::parse(status_str);
        MYLOG_INFO("[API-Light] 探照灯状态获取成功，在线={}", manager.online());
    } catch (const std::exception& e) {
        // 解析失败时原始字符串直接返回
        MYLOG_WARN("[API-Light] 状态 JSON 解析失败，返回原始字符串：{}", e.what());
        data = {{"raw_status", status_str}};
    }

    return jsonOk(data, "探照灯状态获取成功");
}

} // namespace my_api::light_api
