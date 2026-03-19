#pragma once

/**
 * @file pod_scheduler.h
 * @brief 吊舱调度器（骨架）
 * 
 * 预留吊舱任务调度能力，如定时轮询、任务队列等。
 * 当前为骨架实现，后续扩展。
 */

#include <string>

namespace PodModule {

/**
 * @brief 吊舱调度器（骨架）
 * 
 * 预留功能：
 * - 定时心跳调度
 * - 任务队列管理
 * - 定时状态同步
 */
class PodScheduler {
public:
    PodScheduler();
    ~PodScheduler();

    /** @brief 启动调度器 */
    void start();

    /** @brief 停止调度器 */
    void stop();

    /** @brief 调度器是否运行中 */
    bool isRunning() const;

private:
    bool running_ = false;
};

} // namespace PodModule
