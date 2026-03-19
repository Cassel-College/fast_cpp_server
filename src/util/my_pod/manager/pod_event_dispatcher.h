#pragma once

/**
 * @file pod_event_dispatcher.h
 * @brief 吊舱事件分发器（骨架）
 * 
 * 预留吊舱事件分发能力，如状态变更通知、能力回调等。
 * 当前为骨架实现，后续扩展。
 */

#include <string>
#include <functional>
#include <vector>

namespace PodModule {

/**
 * @brief 事件类型枚举
 */
enum class PodEventType {
    CONNECTED,       // 设备已连接
    DISCONNECTED,    // 设备已断开
    STATE_CHANGED,   // 状态变更
    ERROR_OCCURRED,  // 发生错误
};

/**
 * @brief 事件数据
 */
struct PodEvent {
    PodEventType type;
    std::string  pod_id;
    std::string  message;
};

/**
 * @brief 事件回调类型
 */
using PodEventCallback = std::function<void(const PodEvent&)>;

/**
 * @brief 吊舱事件分发器（骨架）
 */
class PodEventDispatcher {
public:
    PodEventDispatcher();
    ~PodEventDispatcher();

    /** @brief 注册事件监听器 */
    void addEventListener(PodEventCallback callback);

    /** @brief 分发事件 */
    void dispatch(const PodEvent& event);

    /** @brief 清空所有监听器 */
    void clearListeners();

private:
    std::vector<PodEventCallback> listeners_;
};

} // namespace PodModule
