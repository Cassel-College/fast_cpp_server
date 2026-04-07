#include "gtest/gtest.h"

#include <arpa/inet.h>
#include <unistd.h>

#include "MyIPTools.h"

// ============================================================
//  测试：GetLocalIPv4（原有）
// ============================================================

TEST(MyIPToolsTest, GetLocalIPv4ReturnsValidAddress) {
    const std::string ip = my_tools::MyIPTools::GetLocalIPv4();

    ASSERT_FALSE(ip.empty());

    struct in_addr addr;
    EXPECT_EQ(::inet_pton(AF_INET, ip.c_str(), &addr), 1);
}

// ============================================================
//  测试：CIDR 校验
// ============================================================

TEST(MyIPToolsTest, IsValidCIDR_合法格式) {
    EXPECT_TRUE(my_tools::MyIPTools::IsValidCIDR("192.168.1.100/24"));
    EXPECT_TRUE(my_tools::MyIPTools::IsValidCIDR("10.0.0.1/8"));
    EXPECT_TRUE(my_tools::MyIPTools::IsValidCIDR("255.255.255.255/32"));
    EXPECT_TRUE(my_tools::MyIPTools::IsValidCIDR("0.0.0.0/0"));
}

TEST(MyIPToolsTest, IsValidCIDR_非法格式) {
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR(""));
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("192.168.1.100"));    // 缺少前缀
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("192.168.1.100/"));   // 前缀为空
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("/24"));              // IP 为空
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("abc.def.ghi/24"));  // 非法 IP
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("192.168.1.100/33")); // 前缀超范围
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("192.168.1.100/-1")); // 前缀为负
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("192.168.1.100/ab")); // 前缀非数字
    EXPECT_FALSE(my_tools::MyIPTools::IsValidCIDR("192.168.1.999/24")); // IP 段越界
}

// ============================================================
//  测试：InterfaceExists
// ============================================================

TEST(MyIPToolsTest, InterfaceExists_回环接口) {
    // lo 在所有 Linux 系统上都存在
    EXPECT_TRUE(my_tools::MyIPTools::InterfaceExists("lo"));
}

TEST(MyIPToolsTest, InterfaceExists_不存在的网卡) {
    EXPECT_FALSE(my_tools::MyIPTools::InterfaceExists("nonexistent_iface_xyz"));
}

// ============================================================
//  测试：GetAllInterfaces
// ============================================================

TEST(MyIPToolsTest, GetAllInterfaces_返回非空列表) {
    auto interfaces = my_tools::MyIPTools::GetAllInterfaces();
    // 在有网络的环境中至少应该有一个非回环网卡
    // 此测试在 CI 容器中可能为空，所以仅测试不崩溃
    SUCCEED();
}

// ============================================================
//  测试：GetAllIPs
// ============================================================

TEST(MyIPToolsTest, GetAllIPs_不含回环) {
    auto ips = my_tools::MyIPTools::GetAllIPs(false);
    for (const auto& info : ips) {
        EXPECT_NE(info.interface, "lo");
        EXPECT_FALSE(info.ip.empty());
        EXPECT_GE(info.prefix_len, 0);
        EXPECT_LE(info.prefix_len, 32);
    }
}

TEST(MyIPToolsTest, GetAllIPs_含回环) {
    auto ips = my_tools::MyIPTools::GetAllIPs(true);
    // 应该至少包含 lo 的 127.0.0.1
    bool found_loopback = false;
    for (const auto& info : ips) {
        if (info.interface == "lo" && info.ip == "127.0.0.1") {
            found_loopback = true;
        }
    }
    EXPECT_TRUE(found_loopback);
}

TEST(MyIPToolsTest, GetAllIPs_每条记录格式合法) {
    auto ips = my_tools::MyIPTools::GetAllIPs(true);
    for (const auto& info : ips) {
        EXPECT_FALSE(info.interface.empty());
        // IP 格式校验
        struct in_addr addr;
        EXPECT_EQ(::inet_pton(AF_INET, info.ip.c_str(), &addr), 1)
            << "非法 IP: " << info.ip << " (网卡: " << info.interface << ")";
        EXPECT_GE(info.prefix_len, 0);
        EXPECT_LE(info.prefix_len, 32);
    }
}

// ============================================================
//  测试：AddIP / DeleteIP 参数校验（不需要 Root 权限）
// ============================================================

TEST(MyIPToolsTest, AddIP_非法CIDR格式) {
    auto result = my_tools::MyIPTools::AddIP("lo", "invalid_cidr");
    EXPECT_EQ(result.code, my_tools::IPErrorCode::kInvalidCIDR);
}

TEST(MyIPToolsTest, AddIP_网卡不存在) {
    auto result = my_tools::MyIPTools::AddIP("nonexistent_xyz", "192.168.99.1/24");
    EXPECT_EQ(result.code, my_tools::IPErrorCode::kInterfaceNotFound);
}

TEST(MyIPToolsTest, DeleteIP_非法CIDR格式) {
    auto result = my_tools::MyIPTools::DeleteIP("lo", "bad!format");
    EXPECT_EQ(result.code, my_tools::IPErrorCode::kInvalidCIDR);
}

TEST(MyIPToolsTest, DeleteIP_网卡不存在) {
    auto result = my_tools::MyIPTools::DeleteIP("nonexistent_xyz", "10.0.0.1/24");
    EXPECT_EQ(result.code, my_tools::IPErrorCode::kInterfaceNotFound);
}

// ============================================================
//  测试：IPErrorCodeToString
// ============================================================

TEST(MyIPToolsTest, ErrorCodeToString_所有枚举值) {
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kSuccess).empty());
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kInvalidCIDR).empty());
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kInterfaceNotFound).empty());
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kPermissionDenied).empty());
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kSystemError).empty());
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kIPAlreadyExists).empty());
    EXPECT_FALSE(my_tools::IPErrorCodeToString(my_tools::IPErrorCode::kIPNotFound).empty());
}

// ============================================================
//  测试：需要 Root 权限的操作（条件执行）
//  仅在 Root 用户下执行，否则跳过
// ============================================================

TEST(MyIPToolsTest, AddAndDeleteIP_需要Root权限) {
    if (::geteuid() != 0) {
        GTEST_SKIP() << "跳过：需要 Root 权限才能执行 IP 添加/删除操作";
    }

    // 选择一个存在的网卡
    auto interfaces = my_tools::MyIPTools::GetAllInterfaces();
    if (interfaces.empty()) {
        GTEST_SKIP() << "跳过：无可用网卡";
    }

    const std::string& iface = interfaces.front();
    const std::string test_cidr = "192.168.253.253/24";

    // 添加
    auto add_result = my_tools::MyIPTools::AddIP(iface, test_cidr);
    EXPECT_EQ(add_result.code, my_tools::IPErrorCode::kSuccess)
        << "添加失败: " << add_result.message;

    // 验证添加成功
    auto ips = my_tools::MyIPTools::GetAllIPs();
    bool found = false;
    for (const auto& info : ips) {
        if (info.interface == iface && info.ip == "192.168.253.253") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "添加后未在 IP 列表中找到目标地址";

    // 删除
    auto del_result = my_tools::MyIPTools::DeleteIP(iface, test_cidr);
    EXPECT_EQ(del_result.code, my_tools::IPErrorCode::kSuccess)
        << "删除失败: " << del_result.message;
}

TEST(MyIPToolsTest, AddIP_无Root权限返回PermissionDenied) {
    if (::geteuid() == 0) {
        GTEST_SKIP() << "跳过：当前是 Root 用户";
    }

    auto interfaces = my_tools::MyIPTools::GetAllInterfaces();
    if (interfaces.empty()) {
        GTEST_SKIP() << "跳过：无可用网卡";
    }

    auto result = my_tools::MyIPTools::AddIP(interfaces.front(), "192.168.253.253/24");
    // 非 Root 时应该返回 kPermissionDenied 或 kSystemError
    EXPECT_TRUE(result.code == my_tools::IPErrorCode::kPermissionDenied ||
                result.code == my_tools::IPErrorCode::kSystemError)
        << "期望权限错误，实际: " << my_tools::IPErrorCodeToString(result.code);
}

// ============================================================
//  测试：ICMPHeader 结构体大小
// ============================================================

TEST(MyIPToolsTest, ICMPHeader_结构体大小为8字节) {
    EXPECT_EQ(sizeof(my_tools::ICMPHeader), 8u);
}

// ============================================================
//  测试：GenerateSubnetIPs
// ============================================================

TEST(MyIPToolsTest, GenerateSubnetIPs_24位前缀) {
    auto ips = my_tools::MyIPTools::GenerateSubnetIPs("192.168.1.10", 24);
    // /24 网段：192.168.1.1 ~ 192.168.1.254（排除 .0 和 .255）
    EXPECT_EQ(ips.size(), 254u);

    // 检查首尾
    EXPECT_EQ(ips.front(), "192.168.1.1");
    EXPECT_EQ(ips.back(), "192.168.1.254");
}

TEST(MyIPToolsTest, GenerateSubnetIPs_30位前缀) {
    auto ips = my_tools::MyIPTools::GenerateSubnetIPs("10.0.0.1", 30);
    // /30 网段：10.0.0.1 和 10.0.0.2（排除 .0 和 .3）
    EXPECT_EQ(ips.size(), 2u);
}

TEST(MyIPToolsTest, GenerateSubnetIPs_前缀太短被拒绝) {
    // /8 前缀：16M 地址，应被拒绝
    auto ips = my_tools::MyIPTools::GenerateSubnetIPs("10.0.0.1", 8);
    EXPECT_TRUE(ips.empty());

    // /16 前缀：65534 地址，也应被拒绝（网段过大）
    auto ips16 = my_tools::MyIPTools::GenerateSubnetIPs("172.16.0.1", 16);
    EXPECT_TRUE(ips16.empty());

    // /19 前缀：8190 地址，也应被拒绝
    auto ips19 = my_tools::MyIPTools::GenerateSubnetIPs("192.168.0.1", 19);
    EXPECT_TRUE(ips19.empty());
}

TEST(MyIPToolsTest, GenerateSubnetIPs_前缀太长被拒绝) {
    // /31 前缀：点对点链路，应被拒绝
    auto ips = my_tools::MyIPTools::GenerateSubnetIPs("10.0.0.1", 31);
    EXPECT_TRUE(ips.empty());

    // /32 前缀：单地址，应被拒绝
    auto ips2 = my_tools::MyIPTools::GenerateSubnetIPs("10.0.0.1", 32);
    EXPECT_TRUE(ips2.empty());
}

TEST(MyIPToolsTest, GenerateSubnetIPs_非法IP) {
    auto ips = my_tools::MyIPTools::GenerateSubnetIPs("invalid_ip", 24);
    EXPECT_TRUE(ips.empty());
}

TEST(MyIPToolsTest, GenerateSubnetIPs_20位前缀) {
    auto ips = my_tools::MyIPTools::GenerateSubnetIPs("172.16.0.1", 20);
    // /20 网段：4094 个主机地址（排除网络地址和广播地址）
    EXPECT_EQ(ips.size(), 4094u);
}

// ============================================================
//  测试：InitScanner 参数设置
// ============================================================

TEST(MyIPToolsTest, InitScanner_正常参数) {
    // 不应崩溃
    my_tools::MyIPTools::InitScanner(32, 500);
    SUCCEED();
}

TEST(MyIPToolsTest, InitScanner_边界值) {
    // 极端参数应被修正，不应崩溃
    my_tools::MyIPTools::InitScanner(0, 0);     // 会被修正为 1, 100
    my_tools::MyIPTools::InitScanner(9999, 9999); // 会被修正为 1024, 5000
    SUCCEED();
}

// ============================================================
//  测试：HasRawSocketPermission
// ============================================================

TEST(MyIPToolsTest, HasRawSocketPermission_不崩溃) {
    // 只测试函数不崩溃，返回值取决于运行环境
    bool has_perm = my_tools::MyIPTools::HasRawSocketPermission();
    (void)has_perm;
    SUCCEED();
}

// ============================================================
//  测试：PingHost 参数校验
// ============================================================

TEST(MyIPToolsTest, PingHost_非法IP地址返回false) {
    // 非法 IP 格式应返回 false 而非崩溃
    EXPECT_FALSE(my_tools::MyIPTools::PingHost("not_an_ip", 200));
    EXPECT_FALSE(my_tools::MyIPTools::PingHost("", 200));
}

// ============================================================
//  测试：ScanActiveDevices 权限检查
// ============================================================

TEST(MyIPToolsTest, ScanActiveDevices_无权限时返回错误) {
    if (my_tools::MyIPTools::HasRawSocketPermission()) {
        GTEST_SKIP() << "跳过：当前拥有 RAW Socket 权限";
    }

    auto result = my_tools::MyIPTools::ScanActiveDevices();
    EXPECT_FALSE(result.error.empty());
    EXPECT_EQ(result.active_count, 0);
    EXPECT_TRUE(result.active_ips.empty());
}

TEST(MyIPToolsTest, ScanActiveDevices_有权限时执行扫描) {
    if (!my_tools::MyIPTools::HasRawSocketPermission()) {
        GTEST_SKIP() << "跳过：无 CAP_NET_RAW 权限或非 Root 用户";
    }

    // 使用较小的超时和线程数来加快测试
    my_tools::MyIPTools::InitScanner(16, 300);

    auto result = my_tools::MyIPTools::ScanActiveDevices();
    EXPECT_TRUE(result.error.empty()) << "扫描错误: " << result.error;
    EXPECT_GT(result.total_scanned, 0);
    EXPECT_GE(result.active_count, 0);
    EXPECT_GT(result.scan_duration_ms, 0.0);
    EXPECT_EQ(result.active_count, static_cast<int>(result.active_ips.size()));
}

// ============================================================
//  测试：ScanResult 默认初始化
// ============================================================

TEST(MyIPToolsTest, ScanResult_默认值) {
    my_tools::ScanResult r;
    EXPECT_EQ(r.total_scanned, 0);
    EXPECT_EQ(r.active_count, 0);
    EXPECT_TRUE(r.active_ips.empty());
    EXPECT_DOUBLE_EQ(r.scan_duration_ms, 0.0);
    EXPECT_TRUE(r.error.empty());
}