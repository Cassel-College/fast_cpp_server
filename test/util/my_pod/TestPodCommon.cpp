/**
 * @file TestPodCommon.cpp
 * @brief 吊舱模块公共类型单元测试
 */

#include "gtest/gtest.h"
#include "common/pod_types.h"
#include "common/pod_models.h"
#include "common/pod_errors.h"
#include "common/pod_result.h"
#include "common/capability_types.h"

using namespace PodModule;

// ============ PodTypes 测试 ============

TEST(PodTypesTest, PodVendorToString) {
    EXPECT_EQ(podVendorToString(PodVendor::DJI), "DJI");
    EXPECT_EQ(podVendorToString(PodVendor::PINLING), "PINLING");
    EXPECT_EQ(podVendorToString(PodVendor::UNKNOWN), "UNKNOWN");
}

TEST(PodTypesTest, PodStateToString) {
    EXPECT_EQ(podStateToString(PodState::DISCONNECTED), "已断开");
    EXPECT_EQ(podStateToString(PodState::CONNECTING), "连接中");
    EXPECT_EQ(podStateToString(PodState::CONNECTED), "已连接");
    EXPECT_EQ(podStateToString(PodState::ERROR), "异常");
}

TEST(PodTypesTest, StreamTypeToString) {
    EXPECT_EQ(streamTypeToString(StreamType::RTSP), "RTSP");
    EXPECT_EQ(streamTypeToString(StreamType::RTMP), "RTMP");
    EXPECT_EQ(streamTypeToString(StreamType::RAW), "RAW");
}

// ============ PodModels 测试 ============

TEST(PodModelsTest, PodInfoDefault) {
    PodInfo info;
    EXPECT_TRUE(info.pod_id.empty());
    EXPECT_TRUE(info.pod_name.empty());
    EXPECT_EQ(info.vendor, PodVendor::UNKNOWN);
    EXPECT_EQ(info.port, 0);
}

TEST(PodModelsTest, PTZPoseValues) {
    PTZPose pose;
    pose.yaw = 90.0;
    pose.pitch = -30.0;
    pose.roll = 0.0;
    EXPECT_DOUBLE_EQ(pose.yaw, 90.0);
    EXPECT_DOUBLE_EQ(pose.pitch, -30.0);
    EXPECT_DOUBLE_EQ(pose.roll, 0.0);
}

TEST(PodModelsTest, PTZCommandValues) {
    PTZCommand cmd;
    cmd.yaw_speed = 45.0;
    cmd.pitch_speed = -15.0;
    cmd.zoom_speed = 10.0;
    EXPECT_DOUBLE_EQ(cmd.yaw_speed, 45.0);
    EXPECT_DOUBLE_EQ(cmd.zoom_speed, 10.0);
}

TEST(PodModelsTest, LaserInfoValues) {
    LaserInfo laser;
    laser.distance = 500.0;
    laser.latitude = 39.9;
    laser.longitude = 116.3;
    laser.altitude = 100.0;
    laser.is_valid = true;
    EXPECT_TRUE(laser.is_valid);
    EXPECT_DOUBLE_EQ(laser.distance, 500.0);
}

// ============ PodErrors 测试 ============

TEST(PodErrorsTest, ErrorCodeDescription) {
    std::string desc = podErrorCodeToString(PodErrorCode::SUCCESS);
    EXPECT_FALSE(desc.empty());

    desc = podErrorCodeToString(PodErrorCode::CONNECTION_FAILED);
    EXPECT_FALSE(desc.empty());

    desc = podErrorCodeToString(PodErrorCode::CONNECTION_TIMEOUT);
    EXPECT_FALSE(desc.empty());
}

// ============ PodResult 测试 ============

TEST(PodResultTest, SuccessResult) {
    auto result = PodResult<int>::success(42);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::SUCCESS);
    ASSERT_TRUE(result.data.has_value());
    EXPECT_EQ(result.data.value(), 42);
}

TEST(PodResultTest, FailureResult) {
    auto result = PodResult<int>::fail(PodErrorCode::CONNECTION_TIMEOUT, "超时了");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::CONNECTION_TIMEOUT);
    EXPECT_EQ(result.message, "超时了");
    EXPECT_FALSE(result.data.has_value());
}

TEST(PodResultTest, VoidSuccess) {
    auto result = PodResult<void>::success();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::SUCCESS);
}

TEST(PodResultTest, VoidFailure) {
    auto result = PodResult<void>::fail(PodErrorCode::CONNECTION_FAILED, "连接失败");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::CONNECTION_FAILED);
}

TEST(PodResultTest, StringResult) {
    auto result = PodResult<std::string>::success("hello");
    EXPECT_TRUE(result.isSuccess());
    ASSERT_TRUE(result.data.has_value());
    EXPECT_EQ(result.data.value(), "hello");
}

// ============ CapabilityTypes 测试 ============

TEST(CapabilityTypesTest, CapabilityTypeToString) {
    EXPECT_EQ(capabilityTypeToString(CapabilityType::STATUS), "状态查询");
    EXPECT_EQ(capabilityTypeToString(CapabilityType::HEARTBEAT), "心跳检测");
    EXPECT_EQ(capabilityTypeToString(CapabilityType::PTZ), "云台控制");
    EXPECT_EQ(capabilityTypeToString(CapabilityType::LASER), "激光测距");
    EXPECT_EQ(capabilityTypeToString(CapabilityType::STREAM), "流媒体");
    EXPECT_EQ(capabilityTypeToString(CapabilityType::IMAGE), "图像抓拍");
    EXPECT_EQ(capabilityTypeToString(CapabilityType::CENTER_MEASURE), "中心点测量");
}

TEST(CapabilityTypesTest, CapabilityTypeToKey) {
    EXPECT_EQ(capabilityTypeToKey(CapabilityType::STATUS), "STATUS");
    EXPECT_EQ(capabilityTypeToKey(CapabilityType::PTZ), "PTZ");
    EXPECT_EQ(capabilityTypeToKey(CapabilityType::CENTER_MEASURE), "CENTER_MEASURE");
}

TEST(CapabilityTypesTest, CapabilityTypeFromString) {
    CapabilityType type = CapabilityType::UNKNOWN;

    EXPECT_TRUE(capabilityTypeFromString("PTZ", type));
    EXPECT_EQ(type, CapabilityType::PTZ);

    EXPECT_TRUE(capabilityTypeFromString("center_measure", type));
    EXPECT_EQ(type, CapabilityType::CENTER_MEASURE);

    EXPECT_TRUE(capabilityTypeFromString("流媒体", type));
    EXPECT_EQ(type, CapabilityType::STREAM);

    EXPECT_FALSE(capabilityTypeFromString("not_supported", type));
}
