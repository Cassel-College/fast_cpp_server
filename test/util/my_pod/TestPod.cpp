/**
 * @file TestPod.cpp
 * @brief 吊舱设备层单元测试
 */

#include "gtest/gtest.h"
#include "pod/dji/dji_pod.h"
#include "pod/pinling/pinling_pod.h"
#include "capability/interface/i_status_capability.h"
#include "capability/interface/i_ptz_capability.h"
#include "capability/interface/i_heartbeat_capability.h"
#include "capability/interface/i_laser_capability.h"
#include <thread>
#include <chrono>

using namespace PodModule;

// ============ DjiPod 测试 ============

class DjiPodTest : public ::testing::Test {
protected:
    std::shared_ptr<DjiPod> pod_;

    void SetUp() override {
        PodInfo info;
        info.pod_id = "dji_test_001";
        info.pod_name = "测试大疆吊舱";
        info.vendor = PodVendor::DJI;
        info.ip_address = "192.168.1.100";
        info.port = 8080;
        pod_ = std::make_shared<DjiPod>(info);
    }
};

TEST_F(DjiPodTest, BasicInfo) {
    EXPECT_EQ(pod_->getPodId(), "dji_test_001");
    EXPECT_EQ(pod_->getPodInfo().vendor, PodVendor::DJI);
    EXPECT_EQ(pod_->getState(), PodState::DISCONNECTED);
}

TEST_F(DjiPodTest, InitializeCapabilities) {
    pod_->initializeCapabilities();

    // DjiPod 应该注册了7种能力
    EXPECT_NE(pod_->getCapability(CapabilityType::STATUS), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::HEARTBEAT), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::PTZ), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::LASER), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::STREAM), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::IMAGE), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::CENTER_MEASURE), nullptr);
}

TEST_F(DjiPodTest, CapabilityTypeCast) {
    pod_->initializeCapabilities();

    auto status = pod_->getCapabilityAs<IStatusCapability>(CapabilityType::STATUS);
    ASSERT_NE(status, nullptr);

    auto ptz = pod_->getCapabilityAs<IPtzCapability>(CapabilityType::PTZ);
    ASSERT_NE(ptz, nullptr);

    auto heartbeat = pod_->getCapabilityAs<IHeartbeatCapability>(CapabilityType::HEARTBEAT);
    ASSERT_NE(heartbeat, nullptr);

    auto laser = pod_->getCapabilityAs<ILaserCapability>(CapabilityType::LASER);
    ASSERT_NE(laser, nullptr);
}

TEST_F(DjiPodTest, CapabilityNames) {
    pod_->initializeCapabilities();

    auto status = pod_->getCapability(CapabilityType::STATUS);
    EXPECT_FALSE(status->getName().empty());

    auto ptz = pod_->getCapability(CapabilityType::PTZ);
    EXPECT_FALSE(ptz->getName().empty());
}

// ============ PinlingPod 测试 ============

class PinlingPodTest : public ::testing::Test {
protected:
    std::shared_ptr<PinlingPod> pod_;

    void SetUp() override {
        PodInfo info;
        info.pod_id = "pinling_test_001";
        info.pod_name = "测试品灵吊舱";
        info.vendor = PodVendor::PINLING;
        info.ip_address = "192.168.1.200";
        info.port = 9090;
        pod_ = std::make_shared<PinlingPod>(info);
    }
};

namespace {

std::shared_ptr<PinlingPod> createPinlingPodWithIp(const std::string& ip_address) {
    PodInfo info;
    info.pod_id = "pinling_status_test";
    info.pod_name = "品凌状态测试吊舱";
    info.vendor = PodVendor::PINLING;
    info.ip_address = ip_address;
    info.port = 9090;
    return std::make_shared<PinlingPod>(info);
}

} // namespace

TEST_F(PinlingPodTest, BasicInfo) {
    EXPECT_EQ(pod_->getPodId(), "pinling_test_001");
    EXPECT_EQ(pod_->getPodInfo().vendor, PodVendor::PINLING);
    EXPECT_EQ(pod_->getState(), PodState::DISCONNECTED);
}

TEST_F(PinlingPodTest, InitializeCapabilities) {
    pod_->initializeCapabilities();

    EXPECT_NE(pod_->getCapability(CapabilityType::STATUS), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::HEARTBEAT), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::PTZ), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::LASER), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::STREAM), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::IMAGE), nullptr);
    EXPECT_NE(pod_->getCapability(CapabilityType::CENTER_MEASURE), nullptr);
}

TEST_F(PinlingPodTest, CapabilityTypeCast) {
    pod_->initializeCapabilities();

    auto status = pod_->getCapabilityAs<IStatusCapability>(CapabilityType::STATUS);
    ASSERT_NE(status, nullptr);

    auto ptz = pod_->getCapabilityAs<IPtzCapability>(CapabilityType::PTZ);
    ASSERT_NE(ptz, nullptr);
}

TEST(PinlingStatusCapabilityTest, ReportsConnectedWhenIpReachable) {
    auto pod = createPinlingPodWithIp("127.0.0.1");
    ASSERT_TRUE(pod->initializeCapabilities().isSuccess());

    auto status = pod->getCapabilityAs<IStatusCapability>(CapabilityType::STATUS);
    ASSERT_NE(status, nullptr);

    auto status_result = status->getStatus();
    ASSERT_TRUE(status_result.isSuccess());
    ASSERT_TRUE(status_result.data.has_value());
    EXPECT_EQ(status_result.data->state, PodState::CONNECTED);
    EXPECT_TRUE(status_result.data->error_msg.empty());
}

TEST(PinlingStatusCapabilityTest, ReportsDisconnectedWhenIpUnreachable) {
    auto pod = createPinlingPodWithIp("203.0.113.1");
    ASSERT_TRUE(pod->initializeCapabilities().isSuccess());

    auto status = pod->getCapabilityAs<IStatusCapability>(CapabilityType::STATUS);
    ASSERT_NE(status, nullptr);

    auto status_result = status->getStatus();
    ASSERT_TRUE(status_result.isSuccess());
    ASSERT_TRUE(status_result.data.has_value());
    EXPECT_EQ(status_result.data->state, PodState::DISCONNECTED);
    EXPECT_FALSE(status_result.data->error_msg.empty());
}

// ============ 能力启用配置测试 ============

TEST(CapabilityEnableTest, OnlyRegistersEnabledCapabilities) {
    auto pod = createPinlingPodWithIp("127.0.0.1");

    // 只启用 STATUS 和 PTZ
    std::set<CapabilityType> enabled = {CapabilityType::STATUS, CapabilityType::PTZ};
    pod->setEnabledCapabilities(enabled);
    pod->initializeCapabilities();

    // 启用的能力应存在
    EXPECT_TRUE(pod->hasCapability(CapabilityType::STATUS));
    EXPECT_TRUE(pod->hasCapability(CapabilityType::PTZ));

    // 未启用的能力不应注册
    EXPECT_FALSE(pod->hasCapability(CapabilityType::HEARTBEAT));
    EXPECT_FALSE(pod->hasCapability(CapabilityType::LASER));
    EXPECT_FALSE(pod->hasCapability(CapabilityType::STREAM));
    EXPECT_FALSE(pod->hasCapability(CapabilityType::IMAGE));
    EXPECT_FALSE(pod->hasCapability(CapabilityType::CENTER_MEASURE));

    // 只有 2 个能力
    EXPECT_EQ(pod->listCapabilities().size(), 2u);
}

TEST(CapabilityEnableTest, EmptySetAllowsAllCapabilities) {
    auto pod = createPinlingPodWithIp("127.0.0.1");

    // 空集合 = 全部允许（向后兼容）
    pod->setEnabledCapabilities({});
    pod->initializeCapabilities();

    EXPECT_EQ(pod->listCapabilities().size(), 7u);
}

// ============ PodMonitor 测试 ============

TEST(PodMonitorTest, DefaultRuntimeStatusIsOffline) {
    auto pod = createPinlingPodWithIp("127.0.0.1");
    ASSERT_TRUE(pod->initializeCapabilities().isSuccess());

    // 未启动监控时，运行时状态为默认值
    auto rt = pod->getRuntimeStatus();
    EXPECT_FALSE(rt.is_online);
    EXPECT_EQ(rt.pod_status.state, PodState::DISCONNECTED);
    EXPECT_EQ(rt.last_update_ms, 0u);
}

TEST(PodMonitorTest, StartAndStopMonitor) {
    auto pod = createPinlingPodWithIp("127.0.0.1");
    ASSERT_TRUE(pod->initializeCapabilities().isSuccess());

    EXPECT_FALSE(pod->isMonitorRunning());

    PodMonitorConfig cfg;
    cfg.poll_interval_ms = 200;
    cfg.status_interval_ms = 100;
    pod->startMonitor(cfg);
    EXPECT_TRUE(pod->isMonitorRunning());

    pod->stopMonitor();
    EXPECT_FALSE(pod->isMonitorRunning());
}

TEST(PodMonitorTest, MonitorDetectsOnlineAfterPolling) {
    auto pod = createPinlingPodWithIp("127.0.0.1");
    ASSERT_TRUE(pod->initializeCapabilities().isSuccess());

    PodMonitorConfig cfg;
    cfg.poll_interval_ms   = 200;
    cfg.status_interval_ms = 200;
    cfg.online_window_size = 2;
    cfg.online_threshold   = 2;
    cfg.enable_ptz_poll    = false;
    cfg.enable_laser_poll  = false;
    cfg.enable_stream_poll = false;

    pod->startMonitor(cfg);

    // 等待足够轮次让滑动窗口填满（2次成功 ping 127.0.0.1）
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    auto rt = pod->getRuntimeStatus();
    EXPECT_TRUE(rt.is_online);
    EXPECT_EQ(rt.pod_status.state, PodState::CONNECTED);
    EXPECT_GT(rt.last_update_ms, 0u);

    pod->stopMonitor();
}

TEST(PodMonitorTest, MonitorDetectsOfflineForUnreachableIp) {
    auto pod = createPinlingPodWithIp("203.0.113.1");
    ASSERT_TRUE(pod->initializeCapabilities().isSuccess());

    PodMonitorConfig cfg;
    cfg.poll_interval_ms   = 200;
    cfg.status_interval_ms = 200;
    cfg.online_window_size = 2;
    cfg.online_threshold   = 2;
    cfg.enable_ptz_poll    = false;
    cfg.enable_laser_poll  = false;
    cfg.enable_stream_poll = false;

    pod->startMonitor(cfg);

    // 等待至少 2 轮 ping（不可达 IP ~1s/次）
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    auto rt = pod->getRuntimeStatus();
    EXPECT_FALSE(rt.is_online);
    EXPECT_EQ(rt.pod_status.state, PodState::DISCONNECTED);

    pod->stopMonitor();
}

TEST(PodMonitorTest, DestructorStopsMonitorGracefully) {
    // Pod 析构时 PodMonitor 应自动停止，不崩溃
    {
        auto pod = createPinlingPodWithIp("127.0.0.1");
        pod->initializeCapabilities();

        PodMonitorConfig cfg;
        cfg.poll_interval_ms   = 200;
        cfg.status_interval_ms = 200;
        pod->startMonitor(cfg);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // pod 超出作用域，析构时自动 stop
    }
    // 到达这里说明没有崩溃/死锁
    SUCCEED();
}
