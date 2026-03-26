#include "gtest/gtest.h"

#include <arpa/inet.h>

#include "MyIPTools.h"

TEST(MyIPToolsTest, GetLocalIPv4ReturnsValidAddress) {
    const std::string ip = my_tools::MyIPTools::GetLocalIPv4();

    ASSERT_FALSE(ip.empty());

    struct in_addr addr;
    EXPECT_EQ(::inet_pton(AF_INET, ip.c_str(), &addr), 1);
}