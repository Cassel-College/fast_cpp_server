#pragma once

/**
 * @file dji_pod.h
 * @brief 大疆吊舱实现
 * 
 * 继承 BasePod，负责：
 * - 初始化大疆 Session
 * - 注册大疆支持的所有 Capability
 * 
 * 不在此类中堆积大量业务逻辑，能力实现在 capability/dji/ 下。
 */

#include "../base/base_pod.h"

namespace PodModule {

class DjiPod : public BasePod {
public:
    /**
     * @brief 构造大疆吊舱
     * @param info 设备基础信息
     */
    explicit DjiPod(const PodInfo& info);
    ~DjiPod() override;

    /**
     * @brief 初始化大疆能力装配
     * 
     * 创建并注册大疆支持的所有能力：
     * - 状态查询
     * - 心跳检测
     * - 云台控制
     * - 激光测距
     * - 流媒体
     * - 图像抓拍
     * - 中心点测量
     */
    PodResult<void> initializeCapabilities() override;
};

} // namespace PodModule
