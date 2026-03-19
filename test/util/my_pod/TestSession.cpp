/**
 * @file TestSession.cpp
 * @brief 会话层单元测试
 */

#include "gtest/gtest.h"
#include "session/dji/dji_session.h"
#include "session/pinling/pinling_session.h"

using namespace PodModule;

TEST(DjiSessionTest, InitialState) {
    DjiSession session;
    EXPECT_FALSE(session.isConnected());
}

TEST(DjiSessionTest, ConnectDisconnect) {
    DjiSession session;

    auto result = session.connect("192.168.1.100", 8080);
    // 骨架实现，连接应返回成功或占位失败
    // 这里主要验证接口不崩溃
    EXPECT_NO_THROW(session.isConnected());

    session.disconnect();
    EXPECT_FALSE(session.isConnected());
}

TEST(PinlingSessionTest, InitialState) {
    PinlingSession session;
    EXPECT_FALSE(session.isConnected());
}

TEST(PinlingSessionTest, ConnectDisconnect) {
    PinlingSession session;

    auto result = session.connect("192.168.1.200", 9090);
    EXPECT_NO_THROW(session.isConnected());

    session.disconnect();
    EXPECT_FALSE(session.isConnected());
}
