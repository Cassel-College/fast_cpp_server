/**
 * @file TestPodEventDispatcher.cpp
 * @brief 事件分发器单元测试
 */

#include "gtest/gtest.h"
#include "manager/pod_event_dispatcher.h"

using namespace PodModule;

TEST(PodEventDispatcherTest, RegisterAndDispatch) {
    PodEventDispatcher dispatcher;
    int callback_count = 0;

    dispatcher.addEventListener([&callback_count](const PodEvent& event) {
        callback_count++;
    });

    PodEvent event;
    event.pod_id = "test_001";
    event.type = PodEventType::CONNECTED;
    event.message = "设备已连接";

    dispatcher.dispatch(event);
    EXPECT_EQ(callback_count, 1);

    dispatcher.dispatch(event);
    EXPECT_EQ(callback_count, 2);
}

TEST(PodEventDispatcherTest, MultipleCallbacks) {
    PodEventDispatcher dispatcher;
    int count_a = 0;
    int count_b = 0;

    dispatcher.addEventListener([&count_a](const PodEvent&) { count_a++; });
    dispatcher.addEventListener([&count_b](const PodEvent&) { count_b++; });

    PodEvent event;
    event.pod_id = "test_001";
    event.type = PodEventType::STATE_CHANGED;

    dispatcher.dispatch(event);
    EXPECT_EQ(count_a, 1);
    EXPECT_EQ(count_b, 1);
}

TEST(PodEventDispatcherTest, EventData) {
    PodEventDispatcher dispatcher;
    std::string received_pod_id;
    PodEventType received_type;

    dispatcher.addEventListener([&](const PodEvent& event) {
        received_pod_id = event.pod_id;
        received_type = event.type;
    });

    PodEvent event;
    event.pod_id = "dji_001";
    event.type = PodEventType::DISCONNECTED;
    event.message = "心跳丢失";

    dispatcher.dispatch(event);
    EXPECT_EQ(received_pod_id, "dji_001");
    EXPECT_EQ(received_type, PodEventType::DISCONNECTED);
}
