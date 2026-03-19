#pragma once

/**
 * @file pinling_pod.h
 * @brief 品凌吊舱实现
 * 
 * 继承 BasePod，负责：
 * - 初始化品凌 Session
 * - 注册品凌支持的所有 Capability
 */

#include "../base/base_pod.h"

namespace PodModule {

class PinlingPod : public BasePod {
public:
    explicit PinlingPod(const PodInfo& info);
    ~PinlingPod() override;

    /**
     * @brief 初始化品凌能力装配
     */
    PodResult<void> initializeCapabilities() override;
};

} // namespace PodModule
