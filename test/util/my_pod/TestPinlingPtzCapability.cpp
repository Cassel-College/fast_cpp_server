/**
 * @file TestPinlingPtzCapability.cpp
 * @brief 品凌吊舱 PTZ 方法单元测试
 */

#include "gtest/gtest.h"
#include "pod/pinling/pinling_pod.h"

using namespace PodModule;

namespace {

std::shared_ptr<PinlingPod> createTestPinlingPod() {
    PodInfo info;
    info.pod_id = "ptz_test_001";
    info.pod_name = "PTZ测试品凌";
    info.vendor = PodVendor::PINLING;
    info.ip_address = "192.168.2.119";
    info.port = 2000;
    return std::make_shared<PinlingPod>(info);
}

} // namespace

TEST(PinlingPtzTest, GetPoseFailsWhenSdkNotConnected) {
    auto pod = createTestPinlingPod();

    auto result = pod->getPose();
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::NOT_CONNECTED);
}

TEST(PinlingPtzTest, SetPoseFailsWhenSdkNotConnected) {
    auto pod = createTestPinlingPod();

    PTZPose pose;
    pose.yaw = 10.0;
    pose.pitch = -5.0;
    auto result = pod->setPose(pose);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::NOT_CONNECTED);
}

TEST(PinlingPtzTest, ControlSpeedFailsWhenSdkNotConnected) {
    auto pod = createTestPinlingPod();

    PTZCommand cmd;
    cmd.yaw_speed = 5.0;
    cmd.pitch_speed = 3.0;
    auto result = pod->controlSpeed(cmd);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::NOT_CONNECTED);
}

TEST(PinlingPtzTest, StopPtzFailsWhenSdkNotConnected) {
    auto pod = createTestPinlingPod();

    auto result = pod->stopPtz();
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::NOT_CONNECTED);
}

TEST(PinlingPtzTest, GoHomeFailsWhenSdkNotConnected) {
    auto pod = createTestPinlingPod();

    auto result = pod->goHome();
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::NOT_CONNECTED);
}

TEST(PinlingPtzTest, InitialSdkIsNotConnected) {
    auto pod = createTestPinlingPod();
    EXPECT_FALSE(pod->isSdkConnected());
}

TEST(PinlingPtzTest, ZoomToFailsWhenSdkNotConnected) {
    auto pod = createTestPinlingPod();
    auto result = pod->zoomTo(5.0f);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::NOT_CONNECTED);
}
