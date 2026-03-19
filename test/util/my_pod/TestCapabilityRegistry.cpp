/**
 * @file TestCapabilityRegistry.cpp
 * @brief 能力注册表单元测试
 */

#include "gtest/gtest.h"
#include "registry/capability_registry.h"
#include "capability/base/base_status_capability.h"
#include "capability/base/base_ptz_capability.h"
#include "capability/base/base_heartbeat_capability.h"
#include "capability/interface/i_status_capability.h"
#include "capability/interface/i_ptz_capability.h"

using namespace PodModule;

class CapabilityRegistryTest : public ::testing::Test {
protected:
    CapabilityRegistry registry_;
};

TEST_F(CapabilityRegistryTest, RegisterAndGet) {
    auto status_cap = std::make_shared<BaseStatusCapability>();
    registry_.registerCapability(CapabilityType::STATUS, status_cap);

    auto cap = registry_.getCapability(CapabilityType::STATUS);
    ASSERT_NE(cap, nullptr);
    EXPECT_EQ(cap->getType(), CapabilityType::STATUS);
}

TEST_F(CapabilityRegistryTest, GetNonExistent) {
    auto cap = registry_.getCapability(CapabilityType::PTZ);
    EXPECT_EQ(cap, nullptr);
}

TEST_F(CapabilityRegistryTest, HasCapability) {
    EXPECT_FALSE(registry_.hasCapability(CapabilityType::STATUS));

    auto status_cap = std::make_shared<BaseStatusCapability>();
    registry_.registerCapability(CapabilityType::STATUS, status_cap);

    EXPECT_TRUE(registry_.hasCapability(CapabilityType::STATUS));
}

TEST_F(CapabilityRegistryTest, GetCapabilityAs) {
    auto status_cap = std::make_shared<BaseStatusCapability>();
    registry_.registerCapability(CapabilityType::STATUS, status_cap);

    auto typed = registry_.getCapabilityAs<IStatusCapability>(CapabilityType::STATUS);
    ASSERT_NE(typed, nullptr);
}

TEST_F(CapabilityRegistryTest, GetCapabilityAsWrongType) {
    auto status_cap = std::make_shared<BaseStatusCapability>();
    registry_.registerCapability(CapabilityType::STATUS, status_cap);

    // 尝试将 STATUS 能力转换为 PTZ 接口，应返回 nullptr
    auto typed = registry_.getCapabilityAs<IPtzCapability>(CapabilityType::STATUS);
    EXPECT_EQ(typed, nullptr);
}

TEST_F(CapabilityRegistryTest, GetAllCapabilities) {
    auto status_cap = std::make_shared<BaseStatusCapability>();
    auto ptz_cap = std::make_shared<BasePtzCapability>();

    registry_.registerCapability(CapabilityType::STATUS, status_cap);
    registry_.registerCapability(CapabilityType::PTZ, ptz_cap);

    auto all = registry_.listCapabilities();
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(CapabilityRegistryTest, RegisterOverwrite) {
    auto cap1 = std::make_shared<BaseStatusCapability>();
    auto cap2 = std::make_shared<BaseStatusCapability>();

    registry_.registerCapability(CapabilityType::STATUS, cap1);
    registry_.registerCapability(CapabilityType::STATUS, cap2);

    // 最后注册的应该覆盖之前的
    auto cap = registry_.getCapability(CapabilityType::STATUS);
    EXPECT_EQ(cap, cap2);
}

TEST_F(CapabilityRegistryTest, UnregisterCapability) {
    auto cap = std::make_shared<BaseStatusCapability>();
    registry_.registerCapability(CapabilityType::STATUS, cap);
    EXPECT_TRUE(registry_.hasCapability(CapabilityType::STATUS));

    registry_.unregisterCapability(CapabilityType::STATUS);
    EXPECT_FALSE(registry_.hasCapability(CapabilityType::STATUS));
}

TEST_F(CapabilityRegistryTest, MultipleCapabilities) {
    auto status_cap = std::make_shared<BaseStatusCapability>();
    auto ptz_cap = std::make_shared<BasePtzCapability>();
    auto heartbeat_cap = std::make_shared<BaseHeartbeatCapability>();

    registry_.registerCapability(CapabilityType::STATUS, status_cap);
    registry_.registerCapability(CapabilityType::PTZ, ptz_cap);
    registry_.registerCapability(CapabilityType::HEARTBEAT, heartbeat_cap);

    EXPECT_TRUE(registry_.hasCapability(CapabilityType::STATUS));
    EXPECT_TRUE(registry_.hasCapability(CapabilityType::PTZ));
    EXPECT_TRUE(registry_.hasCapability(CapabilityType::HEARTBEAT));
    EXPECT_FALSE(registry_.hasCapability(CapabilityType::LASER));

    auto all = registry_.listCapabilities();
    EXPECT_EQ(all.size(), 3u);
}
