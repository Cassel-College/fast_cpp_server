/**
 * @file TestPodRegistry.cpp
 * @brief 设备注册表单元测试
 */

#include "gtest/gtest.h"
#include "registry/pod_registry.h"
#include "pod/dji/dji_pod.h"
#include "pod/pinling/pinling_pod.h"

using namespace PodModule;

class PodRegistryTest : public ::testing::Test {
protected:
    PodRegistry registry_;

    std::shared_ptr<IPod> makeDjiPod(const std::string& id) {
        PodInfo info;
        info.pod_id = id;
        info.pod_name = "测试DJI吊舱_" + id;
        info.vendor = PodVendor::DJI;
        info.ip_address = "192.168.1.100";
        info.port = 8080;
        return std::make_shared<DjiPod>(info);
    }
};

TEST_F(PodRegistryTest, RegisterAndGet) {
    auto pod = makeDjiPod("dji_001");
    registry_.registerPod("dji_001", pod);

    auto retrieved = registry_.getPod("dji_001");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getPodId(), "dji_001");
}

TEST_F(PodRegistryTest, GetNonExistent) {
    auto pod = registry_.getPod("not_exist");
    EXPECT_EQ(pod, nullptr);
}

TEST_F(PodRegistryTest, HasPod) {
    EXPECT_FALSE(registry_.hasPod("dji_001"));
    auto pod = makeDjiPod("dji_001");
    registry_.registerPod("dji_001", pod);
    EXPECT_TRUE(registry_.hasPod("dji_001"));
}

TEST_F(PodRegistryTest, UnregisterPod) {
    auto pod = makeDjiPod("dji_001");
    registry_.registerPod("dji_001", pod);
    EXPECT_TRUE(registry_.hasPod("dji_001"));

    registry_.unregisterPod("dji_001");
    EXPECT_FALSE(registry_.hasPod("dji_001"));
}

TEST_F(PodRegistryTest, ListPods) {
    auto pod1 = makeDjiPod("dji_001");
    auto pod2 = makeDjiPod("dji_002");
    registry_.registerPod("dji_001", pod1);
    registry_.registerPod("dji_002", pod2);

    auto all = registry_.listPods();
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(PodRegistryTest, RegisterDuplicateOverwrites) {
    auto pod1 = makeDjiPod("dji_001");
    auto pod2 = makeDjiPod("dji_001");

    registry_.registerPod("dji_001", pod1);
    registry_.registerPod("dji_001", pod2);

    auto retrieved = registry_.getPod("dji_001");
    EXPECT_EQ(retrieved, pod2);
}
