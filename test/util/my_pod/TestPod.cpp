/**
 * @file TestPod.cpp
 * @brief 吊舱设备层单元测试
 */

#include "gtest/gtest.h"
#include "pod/dji/dji_pod.h"
#include "pod/pinling/pinling_pod.h"
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

TEST_F(DjiPodTest, DeviceStatusReturnsResult) {
    auto result = pod_->getDeviceStatus();
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(DjiPodTest, PtzMethodsReturnResult) {
    auto pose = pod_->getPose();
    EXPECT_TRUE(pose.isSuccess());

    auto stop = pod_->stopPtz();
    EXPECT_TRUE(stop.isSuccess());

    auto home = pod_->goHome();
    EXPECT_TRUE(home.isSuccess());
}

TEST_F(DjiPodTest, LaserMethodsReturnResult) {
    auto enable = pod_->enableLaser();
    EXPECT_TRUE(enable.isSuccess());

    auto measure = pod_->laserMeasure();
    EXPECT_TRUE(measure.isSuccess());
}

TEST_F(DjiPodTest, StreamMethodsReturnResult) {
    auto info = pod_->getStreamInfo();
    EXPECT_TRUE(info.isSuccess());
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

TEST_F(PinlingPodTest, InitSucceedsAndStoresCompleteConfig) {
    nlohmann::json config = {
        {"name", "初始化后的品凌吊舱"},
        {"type", "pinling"},
        {"ip", "10.10.10.20"},
        {"port", 2100},
        {"capability", {
            {"PTZ", {
                {"open", "enable"},
                {"init_args", {
                    {"link_type", "tcp"},
                    {"ip", "10.10.10.21"},
                    {"port", 2200}
                }}
            }}
        }}
    };

    auto result = pod_->Init(config);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_NE(pod_->getSession(), nullptr);

    auto info = pod_->getPodInfo();
    EXPECT_EQ(info.pod_name, "初始化后的品凌吊舱");
    EXPECT_EQ(info.ip_address, "10.10.10.20");
    EXPECT_EQ(info.port, 2100);
    ASSERT_TRUE(info.raw_config.contains("capability"));
    EXPECT_EQ(info.raw_config["capability"]["PTZ"]["init_args"]["port"].get<int>(), 2200);
}

TEST(PinlingInitTest, InitFailsWhenPtzEnabledButConnectionParamsMissing) {
    PodInfo info;
    info.pod_id = "pinling_init_invalid";
    info.pod_name = "无效初始化品凌";
    info.vendor = PodVendor::PINLING;

    auto pod = std::make_shared<PinlingPod>(info);
    nlohmann::json config = {
        {"name", "无效初始化品凌"},
        {"type", "pinling"},
        {"capability", {
            {"PTZ", {
                {"open", "enable"}
            }}
        }}
    };

    auto result = pod->Init(config);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code, PodErrorCode::UNKNOWN_ERROR);
}

TEST_F(PinlingPodTest, PtzMethodsFailWhenNotConnected) {
    auto pose = pod_->getPose();
    EXPECT_FALSE(pose.isSuccess());
    EXPECT_EQ(pose.code, PodErrorCode::NOT_CONNECTED);

    auto stop = pod_->stopPtz();
    EXPECT_FALSE(stop.isSuccess());
    EXPECT_EQ(stop.code, PodErrorCode::NOT_CONNECTED);

    auto home = pod_->goHome();
    EXPECT_FALSE(home.isSuccess());
    EXPECT_EQ(home.code, PodErrorCode::NOT_CONNECTED);
}

TEST(PinlingStatusTest, ReportsConnectedWhenIpReachable) {
    auto pod = createPinlingPodWithIp("127.0.0.1");

    auto status_result = pod->getDeviceStatus();
    ASSERT_TRUE(status_result.isSuccess());
    ASSERT_TRUE(status_result.data.has_value());
    EXPECT_EQ(status_result.data->state, PodState::CONNECTED);
    EXPECT_TRUE(status_result.data->error_msg.empty());
}

TEST(PinlingStatusTest, ReportsDisconnectedWhenIpUnreachable) {
    auto pod = createPinlingPodWithIp("203.0.113.1");

    auto status_result = pod->getDeviceStatus();
    ASSERT_TRUE(status_result.isSuccess());
    ASSERT_TRUE(status_result.data.has_value());
    EXPECT_EQ(status_result.data->state, PodState::DISCONNECTED);
    EXPECT_FALSE(status_result.data->error_msg.empty());
}

// ============ PodMonitor 测试 ============

TEST(PodMonitorTest, DefaultRuntimeStatusIsOffline) {
    auto pod = createPinlingPodWithIp("127.0.0.1");

    auto rt = pod->getRuntimeStatus();
    EXPECT_FALSE(rt.is_online);
    EXPECT_EQ(rt.pod_status.state, PodState::DISCONNECTED);
    EXPECT_EQ(rt.last_update_ms, 0u);
}

TEST(PodMonitorTest, StartAndStopMonitor) {
    auto pod = createPinlingPodWithIp("127.0.0.1");

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

    PodMonitorConfig cfg;
    cfg.poll_interval_ms   = 200;
    cfg.status_interval_ms = 200;
    cfg.online_window_size = 2;
    cfg.online_threshold   = 2;
    cfg.enable_ptz_poll    = false;
    cfg.enable_laser_poll  = false;
    cfg.enable_stream_poll = false;

    pod->startMonitor(cfg);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    auto rt = pod->getRuntimeStatus();
    EXPECT_TRUE(rt.is_online);
    EXPECT_EQ(rt.pod_status.state, PodState::CONNECTED);
    EXPECT_GT(rt.last_update_ms, 0u);

    pod->stopMonitor();
}

TEST(PodMonitorTest, MonitorDetectsOfflineForUnreachableIp) {
    auto pod = createPinlingPodWithIp("203.0.113.1");

    PodMonitorConfig cfg;
    cfg.poll_interval_ms   = 200;
    cfg.status_interval_ms = 200;
    cfg.online_window_size = 2;
    cfg.online_threshold   = 2;
    cfg.enable_ptz_poll    = false;
    cfg.enable_laser_poll  = false;
    cfg.enable_stream_poll = false;

    pod->startMonitor(cfg);

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    auto rt = pod->getRuntimeStatus();
    EXPECT_FALSE(rt.is_online);
    EXPECT_EQ(rt.pod_status.state, PodState::DISCONNECTED);

    pod->stopMonitor();
}

TEST(PodMonitorTest, DestructorStopsMonitorGracefully) {
    {
        auto pod = createPinlingPodWithIp("127.0.0.1");

        PodMonitorConfig cfg;
        cfg.poll_interval_ms   = 200;
        cfg.status_interval_ms = 200;
        pod->startMonitor(cfg);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    SUCCEED();
}
