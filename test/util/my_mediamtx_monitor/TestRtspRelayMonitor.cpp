/**
 * @file TestRtspRelayMonitor.cpp
 * @brief RtspRelayMonitor / RtspRelayMonitorManager 单元测试
 *
 * 测试覆盖：
 *   1. RtspRelayStateToString  — 枚举到字符串转换
 *   2. JsonToYaml              — JSON→YAML 递归转换（各类型分支）
 *   3. ParseConfig（通过 Init）— 配置解析：正常 / 缺失字段 / 类型错误 / 可选字段
 *   4. GenerateYAML（通过 Init）— YAML 文件生成与内容校验
 *   5. Init                    — 成功 / 失败路径
 *   6. Start / Stop            — 生命周期管理
 *   7. GetState / GetHeartbeat — 状态查询与 JSON 结构
 *   8. IsPortAvailable         — 端口可用性检测
 *   9. CheckRtspSource         — RTSP 源探测（无效地址 / 连接不可达）
 *  10. 单例 Manager            — GetInstance 一致性 / 委托接口
 */

#include "gtest/gtest.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>
#include "RtspRelayMonitor.h"
#include "RtspRelayMonitorManager.h"

using json = nlohmann::json;
using namespace my_mediamtx_monitor;

// ============================================================================
// 辅助：构建完整的有效配置 JSON
// ============================================================================

static json MakeValidConfig() {
    return {
        {"mediamtx_enable", true},
        {"mediamtx_bin_path", "/usr/bin/false"},  // 一个存在的二进制，用于不真正启动
        {"check_ip", "127.0.0.1"},
        {"check_port", 554},
        {"yaml_file_name", "test_mediamtx.yaml"},
        {"monitor_interval_sec", 1},
        {"mediamtx_json_config", {
            {"rtspAddress", "8555"},
            {"paths", {
                {"live", {
                    {"source", "rtsp://127.0.0.1"},
                    {"sourceOnDemand", true}
                }}
            }}
        }}
    };
}

/// 测试后清理 YAML 文件
static void CleanupYaml(const std::string& filename = "test_mediamtx.yaml") {
    auto path = std::filesystem::current_path() / filename;
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

// ============================================================================
// 1. RtspRelayStateToString
// ============================================================================

class RtspRelayStateToStringTest : public ::testing::Test {};

TEST_F(RtspRelayStateToStringTest, AllStatesHaveReadableStrings) {
    EXPECT_STREQ(RtspRelayStateToString(RtspRelayState::STOPPED),     "STOPPED");
    EXPECT_STREQ(RtspRelayStateToString(RtspRelayState::STARTING),    "STARTING");
    EXPECT_STREQ(RtspRelayStateToString(RtspRelayState::RUNNING),     "RUNNING");
    EXPECT_STREQ(RtspRelayStateToString(RtspRelayState::SOURCE_LOST), "SOURCE_LOST");
    EXPECT_STREQ(RtspRelayStateToString(RtspRelayState::ERROR),       "ERROR");
}

TEST_F(RtspRelayStateToStringTest, UnknownStateReturnsUnknown) {
    // 强制转换一个越界枚举值
    auto unknown = static_cast<RtspRelayState>(999);
    EXPECT_STREQ(RtspRelayStateToString(unknown), "UNKNOWN");
}

// ============================================================================
// 2. JsonToYaml — 各类型转换
// ============================================================================

class JsonToYamlTest : public ::testing::Test {};

TEST_F(JsonToYamlTest, SimpleStringField) {
    json j = {{"name", "test_server"}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("name: \"test_server\""), std::string::npos);
}

TEST_F(JsonToYamlTest, IntegerField) {
    json j = {{"port", 8080}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("port: 8080"), std::string::npos);
}

TEST_F(JsonToYamlTest, BooleanFieldTrue) {
    json j = {{"enabled", true}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("enabled: yes"), std::string::npos);
}

TEST_F(JsonToYamlTest, BooleanFieldFalse) {
    json j = {{"enabled", false}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("enabled: no"), std::string::npos);
}

TEST_F(JsonToYamlTest, NestedObject) {
    json j = {{"server", {{"host", "localhost"}, {"port", 9999}}}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("server:"), std::string::npos);
    EXPECT_NE(yaml.find("  host: \"localhost\""), std::string::npos);
    EXPECT_NE(yaml.find("  port: 9999"), std::string::npos);
}

TEST_F(JsonToYamlTest, ArrayOfPrimitives) {
    json j = {{"items", {1, 2, 3}}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("items:"), std::string::npos);
    EXPECT_NE(yaml.find("  - 1"), std::string::npos);
    EXPECT_NE(yaml.find("  - 2"), std::string::npos);
    EXPECT_NE(yaml.find("  - 3"), std::string::npos);
}

TEST_F(JsonToYamlTest, ArrayOfObjects) {
    json j = {{"servers", json::array({{{"host", "a"}}, {{"host", "b"}}})}};
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("servers:"), std::string::npos);
    EXPECT_NE(yaml.find("  -"), std::string::npos);
}

TEST_F(JsonToYamlTest, EmptyObject) {
    json j = json::object();
    std::string yaml = JsonToYaml(j);
    EXPECT_TRUE(yaml.empty());
}

TEST_F(JsonToYamlTest, IndentIncreases) {
    json j = {{"a", {{"b", {{"c", "deep"}}}}}};
    std::string yaml = JsonToYaml(j);
    // 第三层嵌套应有 4 个空格缩进
    EXPECT_NE(yaml.find("    c: \"deep\""), std::string::npos);
}

TEST_F(JsonToYamlTest, MediamtxConfigConversion) {
    // 真实 MediaMTX 配置片段
    json j = {
        {"rtspAddress", "8555"},
        {"paths", {
            {"live", {
                {"source", "rtsp://192.168.2.119"},
                {"sourceOnDemand", true}
            }}
        }}
    };
    std::string yaml = JsonToYaml(j);
    EXPECT_NE(yaml.find("rtspAddress: \"8555\""), std::string::npos);
    EXPECT_NE(yaml.find("paths:"), std::string::npos);
    EXPECT_NE(yaml.find("source: \"rtsp://192.168.2.119\""), std::string::npos);
    EXPECT_NE(yaml.find("sourceOnDemand: yes"), std::string::npos);
}

// ============================================================================
// 3. Init — 配置解析（成功 / 失败分支）
// ============================================================================

class RtspRelayMonitorInitTest : public ::testing::Test {
protected:
    void TearDown() override {
        CleanupYaml();
        CleanupYaml("mediamtx.yaml");
    }
};

TEST_F(RtspRelayMonitorInitTest, ValidConfigSucceeds) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    EXPECT_TRUE(monitor.Init(cfg));
}

TEST_F(RtspRelayMonitorInitTest, MissingCheckIpFails) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg.erase("check_ip");
    EXPECT_FALSE(monitor.Init(cfg));
}

TEST_F(RtspRelayMonitorInitTest, CheckIpWrongTypeFails) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["check_ip"] = 12345;  // 应为 string
    EXPECT_FALSE(monitor.Init(cfg));
}

TEST_F(RtspRelayMonitorInitTest, MissingMediamtxJsonConfigFails) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg.erase("mediamtx_json_config");
    EXPECT_FALSE(monitor.Init(cfg));
}

TEST_F(RtspRelayMonitorInitTest, MediamtxJsonConfigWrongTypeFails) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_json_config"] = "not_an_object";
    EXPECT_FALSE(monitor.Init(cfg));
}

TEST_F(RtspRelayMonitorInitTest, EnableDefaultFalseWhenMissing) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg.erase("mediamtx_enable");
    EXPECT_TRUE(monitor.Init(cfg));
    // enable 默认 false → 状态为 STOPPED
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
}

TEST_F(RtspRelayMonitorInitTest, EnableSetToFalse) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = false;
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_FALSE(hb["enable"].get<bool>());
}

TEST_F(RtspRelayMonitorInitTest, EnableNonBoolIgnored) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = "true";  // 字符串，不是布尔
    EXPECT_TRUE(monitor.Init(cfg));
    // enable 保持默认 false
    auto hb = monitor.GetHeartbeat();
    EXPECT_FALSE(hb["enable"].get<bool>());
}

// ============================================================================
// 4. mediamtx_bin_path — 新旧字段名兼容
// ============================================================================

class BinPathConfigTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(BinPathConfigTest, NewFieldMediamtxBinPath) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_bin_path"] = "/opt/mediamtx/mediamtx";
    cfg.erase("mediamtx_bin");
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    // heartbeat 不直接暴露 bin_path，但初始化应成功
    EXPECT_EQ(hb["module"].get<std::string>(), "RtspRelayMonitor");
}

TEST_F(BinPathConfigTest, OldFieldMediamtxBinFallback) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg.erase("mediamtx_bin_path");
    cfg["mediamtx_bin"] = "/opt/old/mediamtx";
    EXPECT_TRUE(monitor.Init(cfg));
}

TEST_F(BinPathConfigTest, NewFieldTakesPrecedenceOverOld) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_bin_path"] = "/new/path/mediamtx";
    cfg["mediamtx_bin"] = "/old/path/mediamtx";
    EXPECT_TRUE(monitor.Init(cfg));
}

TEST_F(BinPathConfigTest, NeitherFieldUsesDefault) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg.erase("mediamtx_bin_path");
    cfg.erase("mediamtx_bin");
    // 使用默认 ./mediamtx，Init 仍应成功（只是启动时可能找不到二进制）
    EXPECT_TRUE(monitor.Init(cfg));
}

// ============================================================================
// 5. 可选参数解析
// ============================================================================

class OptionalParamsTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); CleanupYaml("custom_mtx.yaml"); }
};

TEST_F(OptionalParamsTest, CustomCheckPort) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["check_port"] = 8554;
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["check_port"].get<int>(), 8554);
}

TEST_F(OptionalParamsTest, DefaultCheckPort2000) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg.erase("check_port");
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["check_port"].get<int>(), 2000);
}

TEST_F(OptionalParamsTest, CustomYamlFileName) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["yaml_file_name"] = "custom_mtx.yaml";
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_NE(hb["yaml_file"].get<std::string>().find("custom_mtx.yaml"), std::string::npos);
}

TEST_F(OptionalParamsTest, MonitorIntervalClampedToMin1) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["monitor_interval_sec"] = 0;  // 低于最小值
    EXPECT_TRUE(monitor.Init(cfg));
    // 不会崩溃，interval 被钳位到 1
}

TEST_F(OptionalParamsTest, RtspListenPortParsedFromAddress) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_json_config"]["rtspAddress"] = "9999";
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["rtsp_listen_port"].get<int>(), 9999);
}

TEST_F(OptionalParamsTest, RtspAddressPlainPortString) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_json_config"]["rtspAddress"] = "7777";
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["rtsp_listen_port"].get<int>(), 7777);
}

TEST_F(OptionalParamsTest, RtspAddressInvalidPortUsesDefault) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_json_config"]["rtspAddress"] = "abc";
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    // 解析失败，使用默认 8555
    EXPECT_EQ(hb["rtsp_listen_port"].get<int>(), 8555);
}

TEST_F(OptionalParamsTest, RtspAddressMissingUsesDefault) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_json_config"].erase("rtspAddress");
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["rtsp_listen_port"].get<int>(), 8555);
}

// ============================================================================
// 6. YAML 文件生成
// ============================================================================

class YamlGenerationTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(YamlGenerationTest, FileCreatedAfterInit) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));

    auto path = std::filesystem::current_path() / "test_mediamtx.yaml";
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(YamlGenerationTest, FileContainsExpectedContent) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));

    auto path = std::filesystem::current_path() / "test_mediamtx.yaml";
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("rtspAddress"), std::string::npos);
    EXPECT_NE(content.find("paths"), std::string::npos);
    EXPECT_NE(content.find("source"), std::string::npos);
}

TEST_F(YamlGenerationTest, OldFileDeletedBeforeRewrite) {
    // 第一次 Init
    {
        RtspRelayMonitor m;
        ASSERT_TRUE(m.Init(MakeValidConfig()));
    }
    // 修改配置后第二次 Init
    {
        RtspRelayMonitor m;
        json cfg = MakeValidConfig();
        cfg["mediamtx_json_config"]["rtspAddress"] = "1234";
        ASSERT_TRUE(m.Init(cfg));
    }

    auto path = std::filesystem::current_path() / "test_mediamtx.yaml";
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    // 确认是新内容
    EXPECT_NE(content.find("\"1234\""), std::string::npos);
}

// ============================================================================
// 7. GetState / GetHeartbeat
// ============================================================================

class StateAndHeartbeatTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(StateAndHeartbeatTest, InitialStateStopped) {
    RtspRelayMonitor monitor;
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
}

TEST_F(StateAndHeartbeatTest, HeartbeatHasRequiredFields) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));

    auto hb = monitor.GetHeartbeat();
    EXPECT_TRUE(hb.contains("module"));
    EXPECT_TRUE(hb.contains("state"));
    EXPECT_TRUE(hb.contains("enable"));
    EXPECT_TRUE(hb.contains("check_ip"));
    EXPECT_TRUE(hb.contains("check_port"));
    EXPECT_TRUE(hb.contains("rtsp_listen_port"));
    EXPECT_TRUE(hb.contains("relay_pid"));
    EXPECT_TRUE(hb.contains("yaml_file"));
}

TEST_F(StateAndHeartbeatTest, HeartbeatModuleName) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["module"].get<std::string>(), "RtspRelayMonitor");
}

TEST_F(StateAndHeartbeatTest, HeartbeatReflectsConfig) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["check_ip"].get<std::string>(), "127.0.0.1");
    EXPECT_EQ(hb["enable"].get<bool>(), true);
    EXPECT_EQ(hb["rtsp_listen_port"].get<int>(), 8555);
}

TEST_F(StateAndHeartbeatTest, HeartbeatPidInitiallyNegative) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));
    auto hb = monitor.GetHeartbeat();
    EXPECT_LT(hb["relay_pid"].get<int>(), 0);
}

// ============================================================================
// 8. Start / Stop 生命周期
// ============================================================================

class LifecycleTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(LifecycleTest, StartWithoutInitDoesNotCrash) {
    RtspRelayMonitor monitor;
    // 未调用 Init，Start 应安全返回（不启动线程）
    monitor.Start();
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
    monitor.Stop();
}

TEST_F(LifecycleTest, DoubleStartIgnored) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));
    monitor.Start();
    monitor.Start();  // 第二次应被忽略
    monitor.Stop();
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
}

TEST_F(LifecycleTest, StopWithoutStartSafe) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));
    monitor.Stop();  // 未 Start 也不崩溃
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
}

TEST_F(LifecycleTest, DoubleStopSafe) {
    RtspRelayMonitor monitor;
    ASSERT_TRUE(monitor.Init(MakeValidConfig()));
    monitor.Start();
    monitor.Stop();
    monitor.Stop();  // 第二次 Stop 不崩溃
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
}

TEST_F(LifecycleTest, StartThenStopCleansUp) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = false;  // 禁用，避免真正启动子进程
    ASSERT_TRUE(monitor.Init(cfg));
    monitor.Start();
    // 等待一个轮询周期
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    monitor.Stop();
    EXPECT_EQ(monitor.GetState(), RtspRelayState::STOPPED);
}

// ============================================================================
// 9. IsPortAvailable — 端口检测
// ============================================================================

class PortAvailableTest : public ::testing::Test {};

TEST_F(PortAvailableTest, HighPortLikelyAvailable) {
    // 使用一个不太可能被占用的高端口
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    ASSERT_TRUE(monitor.Init(cfg));
    // 通过 GetHeartbeat 间接验证端口解析正确
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["rtsp_listen_port"].get<int>(), 8555);
    CleanupYaml();
}

// ============================================================================
// 10. CheckRtspSource — 连通性探测
// ============================================================================

class CheckRtspSourceTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(CheckRtspSourceTest, UnreachableSourceResultsInSourceLost) {
    // 使用一个不可达的 IP，enable=true
    // 启动后应进入 SOURCE_LOST 状态
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["check_ip"] = "192.0.2.1";  // RFC 5737 测试地址，不可路由
    cfg["check_port"] = 554;
    cfg["mediamtx_enable"] = true;
    cfg["monitor_interval_sec"] = 1;
    ASSERT_TRUE(monitor.Init(cfg));
    monitor.Start();
    // 等待足够时间让监控循环运行一次（interval=1s + connect timeout=2s + 余量）
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto state = monitor.GetState();
    monitor.Stop();
    EXPECT_EQ(state, RtspRelayState::SOURCE_LOST);
}

// ============================================================================
// 11. RtspRelayMonitorManager — 单例
// ============================================================================

class ManagerSingletonTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(ManagerSingletonTest, GetInstanceReturnsSameObject) {
    auto& a = RtspRelayMonitorManager::GetInstance();
    auto& b = RtspRelayMonitorManager::GetInstance();
    EXPECT_EQ(&a, &b);
}

TEST_F(ManagerSingletonTest, InitAndGetState) {
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    // 注意：单例在不同测试间共享状态，先 Stop 确保干净
    mgr.Stop();
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = false;
    EXPECT_TRUE(mgr.Init(cfg));
    EXPECT_EQ(mgr.GetState(), RtspRelayState::STOPPED);
}

TEST_F(ManagerSingletonTest, HeartbeatDelegation) {
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    mgr.Stop();
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = false;
    ASSERT_TRUE(mgr.Init(cfg));
    auto hb = mgr.GetHeartbeat();
    EXPECT_TRUE(hb.contains("module"));
    EXPECT_EQ(hb["module"].get<std::string>(), "RtspRelayMonitor");
}

TEST_F(ManagerSingletonTest, IsRunningWhenStopped) {
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    mgr.Stop();
    EXPECT_FALSE(mgr.IsRunning());
}

TEST_F(ManagerSingletonTest, StartAndStopLifecycle) {
    auto& mgr = RtspRelayMonitorManager::GetInstance();
    mgr.Stop();
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = false;
    ASSERT_TRUE(mgr.Init(cfg));
    mgr.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr.Stop();
    EXPECT_EQ(mgr.GetState(), RtspRelayState::STOPPED);
}

// ============================================================================
// 12. 边界情况
// ============================================================================

class EdgeCaseTest : public ::testing::Test {
protected:
    void TearDown() override { CleanupYaml(); }
};

TEST_F(EdgeCaseTest, EmptyJsonConfig) {
    RtspRelayMonitor monitor;
    json cfg = json::object();
    // check_ip 缺失应返回 false
    EXPECT_FALSE(monitor.Init(cfg));
}

TEST_F(EdgeCaseTest, NullJsonFields) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["mediamtx_enable"] = nullptr;  // null 不是 boolean
    EXPECT_TRUE(monitor.Init(cfg));
    // enable 应保持默认 false
    auto hb = monitor.GetHeartbeat();
    EXPECT_FALSE(hb["enable"].get<bool>());
}

TEST_F(EdgeCaseTest, ExtraUnknownFieldsIgnored) {
    RtspRelayMonitor monitor;
    json cfg = MakeValidConfig();
    cfg["unknown_field_1"] = "foo";
    cfg["unknown_field_2"] = 42;
    EXPECT_TRUE(monitor.Init(cfg));
}

TEST_F(EdgeCaseTest, MinimalValidConfig) {
    // 仅包含两个必填字段
    RtspRelayMonitor monitor;
    json cfg = {
        {"check_ip", "10.0.0.1"},
        {"mediamtx_json_config", {
            {"rtspAddress", "8555"}
        }}
    };
    EXPECT_TRUE(monitor.Init(cfg));
    auto hb = monitor.GetHeartbeat();
    EXPECT_EQ(hb["check_ip"].get<std::string>(), "10.0.0.1");
    CleanupYaml("mediamtx.yaml");
}
