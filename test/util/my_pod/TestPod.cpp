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
#include "capability/interface/i_stream_capability.h"
#include "capability/interface/i_image_capability.h"
#include "capability/interface/i_center_measure_capability.h"

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
