/**
 * @file TestPodManager.cpp
 * @brief 吊舱管理器单元测试
 */

#include "gtest/gtest.h"
#include "manager/pod_manager.h"
#include "pod/dji/dji_pod.h"
#include "pod/pinling/pinling_pod.h"

using namespace PodModule;

class PodManagerTest : public ::testing::Test {
protected:
    PodManager manager_;

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
};

TEST_F(PodManagerTest, AddAndGetPod) {
    auto pod = makeDjiPod("dji_001");
    manager_.addPod(pod);

    auto retrieved = manager_.getPod("dji_001");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getPodId(), "dji_001");
}

TEST_F(PodManagerTest, GetNonExistent) {
    EXPECT_EQ(manager_.getPod("nope"), nullptr);
}

TEST_F(PodManagerTest, RemovePod) {
    manager_.addPod(makeDjiPod("dji_001"));
    EXPECT_NE(manager_.getPod("dji_001"), nullptr);

    manager_.removePod("dji_001");
    EXPECT_EQ(manager_.getPod("dji_001"), nullptr);
}

TEST_F(PodManagerTest, ListPods) {
    manager_.addPod(makeDjiPod("dji_001"));
    manager_.addPod(makePinlingPod("pinling_001"));

    auto list = manager_.listPods();
    EXPECT_EQ(list.size(), 2u);
}

TEST_F(PodManagerTest, MultiplePods) {
    manager_.addPod(makeDjiPod("dji_001"));
    manager_.addPod(makeDjiPod("dji_002"));
    manager_.addPod(makePinlingPod("pinling_001"));

    EXPECT_NE(manager_.getPod("dji_001"), nullptr);
    EXPECT_NE(manager_.getPod("dji_002"), nullptr);
    EXPECT_NE(manager_.getPod("pinling_001"), nullptr);

    auto list = manager_.listPods();
    EXPECT_EQ(list.size(), 3u);
}
