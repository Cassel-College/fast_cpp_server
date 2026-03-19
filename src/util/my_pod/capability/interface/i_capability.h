#pragma once

/**
 * @file i_capability.h
 * @brief 能力基础接口定义
 * 
 * 所有吊舱能力的顶层抽象接口。
 * 每个能力都有类型标识、初始化和名称方法。
 */

#include "../../common/capability_types.h"
#include "../../common/pod_result.h"
#include <string>
#include <memory>

namespace PodModule {

// 前向声明
class IPod;
class ISession;

/**
 * @brief 能力基础接口
 * 
 * 所有具体能力接口（如 IPtzCapability、ILaserCapability）都继承自此接口。
 */
class ICapability {
public:
    virtual ~ICapability() = default;

    /** @brief 获取能力类型 */
    virtual CapabilityType getType() const = 0;

    /** @brief 获取能力名称（中文） */
    virtual std::string getName() const = 0;

    /** @brief 初始化能力 */
    virtual PodResult<void> initialize() = 0;

    /** @brief 关联 Pod 设备 */
    virtual void attachPod(IPod* pod) = 0;

    /** @brief 关联 Session 会话 */
    virtual void attachSession(std::shared_ptr<ISession> session) = 0;
};

} // namespace PodModule
