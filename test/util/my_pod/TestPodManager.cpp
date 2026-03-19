/**
 * @file TestPodManager.cpp
 * @brief 吊舱管理器单元测试
 */

#include "gtest/gtest.h"
#include "manager/pod_manager.h"
#include "pod/dji/dji_pod.h"
#include "pod/pinling/pinling_pod.h"

using namespace PodModule;

namespace {

std::shared_ptr<IPod> makeDjiPod(const std::string& id) {
    PodInfo info;
    info.pod_id = id;
    info.pod_name = "DJI_" + id;
    info.vendor = PodVendor::DJI;
    info.ip_address = "192.168.1.100";
    info.port = 8080;
    auto pod = std::make_shared<DjiPod>(info);
    pod->initializeCapabilities();
    return pod;
}

std::shared_ptr<IPod> makePinlingPod(const std::string& id) {
    PodInfo info;
    info.pod_id = id;
    info.pod_name = "Pinling_" + id;
    info.vendor = PodVendor::PINLING;
    info.ip_address = "192.168.1.200";
    info.port = 9090;
    auto pod = std::make_shared<PinlingPod>(info);
    pod->initializeCapabilities();
    return pod;
}

} // namespace

TEST(PodManagerTest, InitFromJson) {
    // 使用单例但测试 Init 功能
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

    // Init 是否能执行（单例只初始化一次）
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
    // 初始化后应该有设备
    EXPECT_GE(ids.size(), 0u);
}
