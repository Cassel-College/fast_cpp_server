#pragma once

/**
 * @file base_capability.h
 * @brief 能力基础实现类
 * 
 * 所有具体能力实现（如 BasePtzCapability）的基类。
 * 提供通用的 Pod 关联、Session 关联、初始化、日志等功能。
 */

#include "../interface/i_capability.h"
#include "../../session/interface/i_session.h"
#include <memory>

namespace PodModule {

// 前向声明
class IPod;

/**
 * @brief 能力基础实现类
 * 
 * 持有 Pod 和 Session 的引用/指针，提供通用初始化逻辑和日志框架。
 * 所有 Base*Capability 和厂商 Capability 都继承此类。
 */
class BaseCapability : virtual public ICapability {
public:
    BaseCapability();
    ~BaseCapability() override;

    // --- ICapability 接口默认实现 ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override;
    void attachSession(std::shared_ptr<ISession> session) override;

protected:
    /** @brief 获取关联的 Pod 指针 */
    IPod* getPod() const;

    /** @brief 获取关联的 Session */
    std::shared_ptr<ISession> getSession() const;

    /** @brief 检查 Session 是否可用 */
    bool isSessionReady() const;

    /** @brief 能力日志前缀（子类可覆盖） */
    virtual std::string logPrefix() const;

    IPod*                    pod_ = nullptr;     // 关联的 Pod 设备
    std::shared_ptr<ISession> session_;           // 关联的会话
};

} // namespace PodModule
