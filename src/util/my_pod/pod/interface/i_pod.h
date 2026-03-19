#pragma once

/**
 * @file i_pod.h
 * @brief 吊舱统一抽象接口
 * 
 * 定义所有吊舱设备的统一抽象接口。
 * BasePod 和厂商 Pod 都必须实现此接口。
 */

#include "../../common/pod_types.h"
#include "../../common/pod_models.h"
#include "../../common/pod_result.h"
#include "../../common/capability_types.h"
#include "../../capability/interface/i_capability.h"
#include "../../session/interface/i_session.h"
#include <string>
#include <memory>
#include <vector>

namespace PodModule {

/**
 * @brief 吊舱统一抽象接口
 * 
 * 所有吊舱设备的顶层接口，定义：
 * - 设备基础信息获取
 * - 连接/断开管理
 * - 状态查询
 * - 能力注册与查询
 * - Session 管理
 * - 能力装配入口
 */
class IPod {
public:
    virtual ~IPod() = default;

    // ==================== 设备基础信息 ====================

    /** @brief 获取设备信息 */
    virtual PodInfo getPodInfo() const = 0;

    /** @brief 获取设备唯一标识 */
    virtual std::string getPodId() const = 0;

    /** @brief 获取设备名称 */
    virtual std::string getPodName() const = 0;

    /** @brief 获取厂商 */
    virtual PodVendor getVendor() const = 0;

    // ==================== 连接管理 ====================

    /** @brief 连接设备 */
    virtual PodResult<void> connect() = 0;

    /** @brief 断开设备 */
    virtual PodResult<void> disconnect() = 0;

    /** @brief 判断是否已连接 */
    virtual bool isConnected() const = 0;

    /** @brief 获取设备当前状态 */
    virtual PodState getState() const = 0;

    // ==================== 能力管理 ====================

    /** @brief 注册一个能力 */
    virtual bool registerCapability(CapabilityType type, std::shared_ptr<ICapability> capability) = 0;

    /** @brief 查询某类型能力 */
    virtual std::shared_ptr<ICapability> getCapability(CapabilityType type) const = 0;

    /**
     * @brief 模板方式查询具体能力接口
     * 
     * 使用方式：
     *   auto ptz = pod->getCapabilityAs<IPtzCapability>(CapabilityType::PTZ);
     */
    template<typename T>
    std::shared_ptr<T> getCapabilityAs(CapabilityType type) const {
        auto cap = getCapability(type);
        if (!cap) return nullptr;
        return std::dynamic_pointer_cast<T>(cap);
    }

    /** @brief 判断是否拥有某能力 */
    virtual bool hasCapability(CapabilityType type) const = 0;

    /** @brief 列出所有已注册能力类型 */
    virtual std::vector<CapabilityType> listCapabilities() const = 0;

    // ==================== Session 管理 ====================

    /** @brief 设置会话 */
    virtual void setSession(std::shared_ptr<ISession> session) = 0;

    /** @brief 获取会话 */
    virtual std::shared_ptr<ISession> getSession() const = 0;

    // ==================== 初始化 ====================

    /**
     * @brief 初始化能力装配
     * 
     * 由厂商 Pod 覆盖实现，负责创建并注册该厂商支持的所有能力。
     */
    virtual PodResult<void> initializeCapabilities() = 0;
};

} // namespace PodModule
