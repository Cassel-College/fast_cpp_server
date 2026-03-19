#pragma once

/**
 * @file capability_registry.h
 * @brief 能力注册表
 * 
 * 管理吊舱设备注册的所有能力，提供注册、查询、枚举等功能。
 * 每个 BasePod 实例持有一个 CapabilityRegistry。
 */

#include "../common/capability_types.h"
#include "../capability/interface/i_capability.h"
#include <unordered_map>
#include <memory>
#include <vector>

namespace PodModule {

/**
 * @brief 能力注册表
 * 
 * 负责管理吊舱设备的所有能力实例：
 * - 注册能力
 * - 通过 CapabilityType 查询能力
 * - 支持模板方式查询具体能力接口
 * - 判断能力是否存在
 * - 列出全部已注册能力
 */
class CapabilityRegistry {
public:
    CapabilityRegistry();
    ~CapabilityRegistry();

    /**
     * @brief 注册一个能力
     * @param type 能力类型
     * @param capability 能力实例（shared_ptr）
     * @return 是否注册成功
     */
    bool registerCapability(CapabilityType type, std::shared_ptr<ICapability> capability);

    /**
     * @brief 注销一个能力
     * @param type 能力类型
     * @return 是否注销成功
     */
    bool unregisterCapability(CapabilityType type);

    /**
     * @brief 根据类型查询能力（返回基础接口指针）
     * @param type 能力类型
     * @return 能力实例，不存在返回 nullptr
     */
    std::shared_ptr<ICapability> getCapability(CapabilityType type) const;

    /**
     * @brief 模板方式查询具体能力接口
     * 
     * 使用方式：
     *   auto ptz = registry.getCapabilityAs<IPtzCapability>(CapabilityType::PTZ);
     * 
     * @tparam T 目标能力接口类型
     * @param type 能力类型
     * @return 转换后的能力接口指针，失败返回 nullptr
     */
    template<typename T>
    std::shared_ptr<T> getCapabilityAs(CapabilityType type) const {
        auto cap = getCapability(type);
        if (!cap) return nullptr;
        return std::dynamic_pointer_cast<T>(cap);
    }

    /**
     * @brief 判断是否存在某能力
     */
    bool hasCapability(CapabilityType type) const;

    /**
     * @brief 获取所有已注册能力类型列表
     */
    std::vector<CapabilityType> listCapabilities() const;

    /**
     * @brief 获取已注册能力数量
     */
    size_t size() const;

    /**
     * @brief 清空所有能力
     */
    void clear();

private:
    std::unordered_map<CapabilityType, std::shared_ptr<ICapability>> capabilities_;
};

} // namespace PodModule
