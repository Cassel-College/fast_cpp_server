/**
 * @file pod_scheduler.cpp
 * @brief 吊舱调度器实现（骨架）
 */

#include "pod_scheduler.h"
#include "MyLog.h"

namespace PodModule {

PodScheduler::PodScheduler() {
    MYLOG_DEBUG("[吊舱调度器] PodScheduler 构造");
}

PodScheduler::~PodScheduler() {
    MYLOG_DEBUG("[吊舱调度器] PodScheduler 析构");
    if (running_) {
        stop();
    }
}

void PodScheduler::start() {
    MYLOG_INFO("[吊舱调度器] 启动调度器");
    running_ = true;
    // TODO: 启动调度线程
}

void PodScheduler::stop() {
    MYLOG_INFO("[吊舱调度器] 停止调度器");
    running_ = false;
    // TODO: 停止调度线程
}

bool PodScheduler::isRunning() const {
    return running_;
}

} // namespace PodModule
