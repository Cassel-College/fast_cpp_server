#include "gtest/gtest.h"

#include <string>

#include <nlohmann/json.hpp>

#include "MyFlyControlManager.h"

using namespace fly_control;

// =============================================================================
// 飞控管理器单元测试
//
// 测试重点：
//   1. 单例入口是否稳定
//   2. 生命周期守卫是否正确
//   3. Shutdown 是否能恢复到干净状态
//   4. 在无真实串口硬件条件下，Init/Send 的失败路径是否可控
// =============================================================================

class MyFlyControlManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        MyFlyControlManager::GetInstance().Shutdown();
    }

    void TearDown() override {
        MyFlyControlManager::GetInstance().Shutdown();
    }

    static nlohmann::json BuildFakeSerialConfig() {
        return {
            {"port", "/dev/ttyFAKE0"},
            {"baudrate", 115200},
            {"timeout_ms", 100},
            {"auto_open", false}
        };
    }
};

TEST_F(MyFlyControlManagerTest, GetInstanceReturnsSameObject) {
    auto& mgr1 = MyFlyControlManager::GetInstance();
    auto& mgr2 = MyFlyControlManager::GetInstance();

    EXPECT_EQ(&mgr1, &mgr2);
}

TEST_F(MyFlyControlManagerTest, DefaultStateAfterShutdownIsClean) {
    auto& mgr = MyFlyControlManager::GetInstance();

    EXPECT_FALSE(mgr.IsInitialized());
    EXPECT_FALSE(mgr.IsRunning());
    EXPECT_FALSE(mgr.HasHeartbeat());
}

TEST_F(MyFlyControlManagerTest, StartBeforeInitFails) {
    auto& mgr = MyFlyControlManager::GetInstance();

    std::string err;
    EXPECT_FALSE(mgr.Start(&err));
    EXPECT_FALSE(err.empty());
}

TEST_F(MyFlyControlManagerTest, InitWithInvalidConfigFailsAndKeepsUninitialized) {
    auto& mgr = MyFlyControlManager::GetInstance();

    std::string err;
    EXPECT_FALSE(mgr.Init(nlohmann::json::object(), &err));
    EXPECT_FALSE(err.empty());
    EXPECT_FALSE(mgr.IsInitialized());
}

TEST_F(MyFlyControlManagerTest, InitWithAutoOpenFalseSucceeds) {
    auto& mgr = MyFlyControlManager::GetInstance();

    std::string err;
    EXPECT_TRUE(mgr.Init(BuildFakeSerialConfig(), &err)) << err;
    EXPECT_TRUE(mgr.IsInitialized());
    EXPECT_FALSE(mgr.IsRunning());
}

TEST_F(MyFlyControlManagerTest, SendBeforePortOpenFailsGracefullyAfterInit) {
    auto& mgr = MyFlyControlManager::GetInstance();

    std::string err;
    ASSERT_TRUE(mgr.Init(BuildFakeSerialConfig(), &err)) << err;

    err.clear();
    EXPECT_FALSE(mgr.SendSetSpeed(1500, &err));
    EXPECT_FALSE(err.empty());
}

TEST_F(MyFlyControlManagerTest, ShutdownResetsInitializedState) {
    auto& mgr = MyFlyControlManager::GetInstance();

    std::string err;
    ASSERT_TRUE(mgr.Init(BuildFakeSerialConfig(), &err)) << err;
    EXPECT_TRUE(mgr.IsInitialized());

    EXPECT_NO_THROW(mgr.SetOnHeartbeat([](const HeartbeatData&) {}));
    EXPECT_NO_THROW(mgr.SetOnCommandReply([](const CommandReplyData&) {}));
    EXPECT_NO_THROW(mgr.SetOnGimbalControl([](const GimbalControlData&) {}));

    mgr.Shutdown();

    EXPECT_FALSE(mgr.IsInitialized());
    EXPECT_FALSE(mgr.IsRunning());
    EXPECT_FALSE(mgr.HasHeartbeat());
}