#pragma once

/**
 * @file base_pod.h
 * @brief 基础吊舱实现
 * 
 * BasePod 是所有厂商 Pod 的基类，作为"轻量设备壳对象"：
 * - 保存设备元信息（PodInfo）
 * - 保存当前设备状态（PodState）
 * - 持有 Session 会话对象
 * - 持有 CapabilityRegistry 能力注册表
 * - 提供基础 connect / disconnect 逻辑
 * - 提供能力注册 / 获取
 * - 提供初始化装配逻辑框架
 * 
 * 不做大而全业务实现——PTZ、测距、拉流等都在 Capability 层。
 */

#include "../interface/i_pod.h"
#include "../../registry/capability_registry.h"
#include "../../session/interface/i_session.h"
#include <memory>

namespace PodModule {

class BasePod : public IPod {
public:
    /**
     * @brief 构造 BasePod
     * @param info 设备基础信息
     */
    explicit BasePod(const PodInfo& info);
    ~BasePod() override;

    // ==================== IPod 接口实现 ====================

    PodInfo getPodInfo() const override;
    std::string getPodId() const override;
    std::string getPodName() const override;
    PodVendor getVendor() const override;

    PodResult<void> connect() override;
    PodResult<void> disconnect() override;
    bool isConnected() const override;
    PodState getState() const override;

    bool registerCapability(CapabilityType type, std::shared_ptr<ICapability> capability) override;
    std::shared_ptr<ICapability> getCapability(CapabilityType type) const override;
    bool hasCapability(CapabilityType type) const override;
    std::vector<CapabilityType> listCapabilities() const override;

    void setSession(std::shared_ptr<ISession> session) override;
    std::shared_ptr<ISession> getSession() const override;

    PodResult<void> initializeCapabilities() override;

protected:
    /** @brief 设置设备状态 */
    void setState(PodState state);

    /**
     * @brief 注册并装配一个能力
     * 
     * 自动完成 attachPod 和 attachSession 绑定。
     * 厂商 Pod 在 initializeCapabilities() 中调用此方法。
     */
    bool addCapability(CapabilityType type, std::shared_ptr<ICapability> capability);

    PodInfo             pod_info_;           // 设备基础信息
    PodState            state_ = PodState::DISCONNECTED;  // 当前设备状态
    std::shared_ptr<ISession> session_;      // 会话对象
    CapabilityRegistry  capability_registry_; // 能力注册表
};

} // namespace PodModule
