/**
 * @file TestPodManager.cpp
 * @brief 吊舱管理器单元测试
 */

#include "gtest/gtest.h"
#include "manager/pod_manager.h"
#include "pod/dji/dji_pod.h"
#include "pod/pinling/pinling_pod.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

using namespace PodModule;

namespace {

std::shared_ptr<IPod> makeDjiPod(const std::string& id) {
    PodInfo info;
    info.pod_id = id;
    info.pod_name = "DJI_" + id;
    info.vendor = PodVendor::DJI;
    info.ip_address = "192.168.1.100";
    info.port = 8080;
    return std::make_shared<DjiPod>(info);
}

std::shared_ptr<IPod> makePinlingPod(const std::string& id) {
    PodInfo info;
    info.pod_id = id;
    info.pod_name = "Pinling_" + id;
    info.vendor = PodVendor::PINLING;
    info.ip_address = "192.168.1.200";
    info.port = 9090;
    return std::make_shared<PinlingPod>(info);
}

} // namespace

TEST(PodManagerTest, InitFromJson) {
    auto& manager = PodManager::GetInstance();

    nlohmann::json config;
    config["pod_args"]["test_dji"] = {
        {"name", "测试大疆"}, {"type", "dji"},
        {"ip", "10.0.0.1"}, {"port", 8080}
    };
    config["pod_args"]["test_pinling"] = {
        {"name", "测试品凌"}, {"type", "pinling"},
        {"ip", "10.0.0.2"}, {"port", 9090}
    };

    bool result = manager.Init(config);
    EXPECT_TRUE(result);
    EXPECT_TRUE(manager.IsInitialized());
}

TEST(PodManagerTest, GetStatusSnapshot) {
    auto& manager = PodManager::GetInstance();
    auto snapshot = manager.GetStatusSnapshot();
    EXPECT_TRUE(snapshot.is_array());
}

TEST(PodManagerTest, ListPodIds) {
    auto& manager = PodManager::GetInstance();
    auto ids = manager.listPodIds();
    EXPECT_GE(ids.size(), 0u);
}

TEST(PodManagerTest, BuildPinlingPodFromJsonAndPrintOnlineAndPoseForTenSeconds) {
    auto& manager = PodManager::GetInstance();
    manager.ResetForTest();

    nlohmann::json config = {
        {"pod_args", {
            {"pod_001", {
                {"name", "pinling_pod_001"},
                {"type", "pinling"},
                {"ip", "192.168.2.119"},
                {"port", 2000},
                {"rtsp_url", "rtsp://192.168.2.119"},
                {"monitor", {
                    {"poll_interval_ms", 500},
                    {"status_interval_ms", 3000},
                    {"ptz_interval_ms", 1000},
                    {"online_window_size", 5},
                    {"online_threshold", 3},
                    {"auto_start", true}
                }},
                {"capability", {
                    {"STATUS", {{"open", "enable"}, {"description", "吊舱状态监测"}}},
                    {"HEARTBEAT", {{"open", "enable"}, {"description", "吊舱心跳检测"}}},
                    {"PTZ", {{"open", "enable"}, {"description", "云台控制能力"}}},
                    {"LASER", {{"open", "unable"}, {"description", "激光测距能力"}}},
                    {"STREAM", {{"open", "unable"}, {"description", "流媒体能力"}}},
                    {"IMAGE", {{"open", "unable"}, {"description", "图像处理能力"}}},
                    {"CENTER_MEASURE", {{"open", "unable"}, {"description", "中心测量能力"}}}
                }},
                {"description", "PINLING吊舱,提供视频流和云台控制"}
            }}
        }}
    };

    ASSERT_TRUE(manager.Init(config));

    auto pod = manager.getPod("pod_001");
    ASSERT_NE(pod, nullptr);

    auto connect_result = manager.connectPod("pod_001");
    ASSERT_TRUE(connect_result.isSuccess());

    const PTZPose default_pose{};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    int sample_count = 0;

    std::cout << "[PodManagerTest] 开始连续 10 秒输出在线状态与云台姿态" << std::endl;
    std::cout << std::fixed << std::setprecision(2);

    while (std::chrono::steady_clock::now() < deadline) {
        const auto runtime_status = pod->getRuntimeStatus();
        const bool is_online = runtime_status.is_online || pod->isConnected();

        auto pose_result = pod->getPose();
        ASSERT_TRUE(pose_result.isSuccess());
        ASSERT_TRUE(pose_result.data.has_value());

        const auto& pose = pose_result.data.value();

        std::cout
            << "[PodManagerTest] sample=" << sample_count
            << " online=" << (is_online ? "true" : "false")
            << " yaw=" << pose.yaw
            << " pitch=" << pose.pitch
            << " roll=" << pose.roll
            << " zoom=" << pose.zoom
            << std::endl;

        if (!is_online) {
            EXPECT_DOUBLE_EQ(pose.yaw, default_pose.yaw);
            EXPECT_DOUBLE_EQ(pose.pitch, default_pose.pitch);
            EXPECT_DOUBLE_EQ(pose.roll, default_pose.roll);
            EXPECT_DOUBLE_EQ(pose.zoom, default_pose.zoom);
        }

        ++sample_count;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    EXPECT_GE(sample_count, 10);

    EXPECT_TRUE(manager.disconnectPod("pod_001").isSuccess());
    manager.ResetForTest();
}
