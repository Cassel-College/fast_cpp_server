/**
 * @file pod_event_dispatcher.cpp
 * @brief 吊舱事件分发器实现（骨架）
 */

#include "pod_event_dispatcher.h"
#include "MyLog.h"

namespace PodModule {

PodEventDispatcher::PodEventDispatcher() {
    MYLOG_DEBUG("[事件分发器] PodEventDispatcher 构造");
}

PodEventDispatcher::~PodEventDispatcher() {
    MYLOG_DEBUG("[事件分发器] PodEventDispatcher 析构");
}

void PodEventDispatcher::addEventListener(PodEventCallback callback) {
    listeners_.push_back(std::move(callback));
    MYLOG_DEBUG("[事件分发器] 注册事件监听器，当前共 {} 个", listeners_.size());
}

void PodEventDispatcher::dispatch(const PodEvent& event) {
    MYLOG_DEBUG("[事件分发器] 分发事件: pod_id={}, message={}", event.pod_id, event.message);
    for (const auto& listener : listeners_) {
        listener(event);
    }
}

void PodEventDispatcher::clearListeners() {
    MYLOG_INFO("[事件分发器] 清空所有监听器");
    listeners_.clear();
}

} // namespace PodModule
