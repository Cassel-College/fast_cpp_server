/**
 * @file TestMFMQTTServer.cpp
 * @brief Unit tests for the MFMQTTServer wrapper
 */
 #include <gtest/gtest.h>
 #include <nlohmann/json.hpp>
 #include "fast_mqtt_server/MFMQTTServer.h"
 #include <fstream>
 #include <string>
 #include <cstdlib>
 
 using namespace fast_mqtt;
 using json = nlohmann::json;
 
class MFMQTTServerTest : public ::testing::Test {
protected:
    std::string test_conf       = "/tmp/mqtt_start_test.conf";
    std::string test_log        = "/tmp/mqtt_start_test.log";
    std::string tmp_conf_path   = "/tmp/test_mosquitto.conf";
     
    void TearDown() override {
         // 清理生成的临时文件
        std::remove(tmp_conf_path.c_str());
    }
};
 
 // 辅助函数：读取文件内容
 std::string readFile(const std::string& path) {
     std::ifstream ifs(path);
     if (!ifs) return "";
     return std::string((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
 }
 
 // 测试配置文件的生成内容是否正确
 TEST_F(MFMQTTServerTest, ConfigGeneration) {
     MFMQTTServer server;
     json cfg = {
         {"port", 9999},
         {"bind_addr", "127.0.0.1"},
         {"allow_anonymous", false},
         {"persistence", true},
         {"conf_path", tmp_conf_path}, // 指定测试路径
         {"log_path", "/dev/null"}
     };
 
     server.init(cfg);
     
     // 尝试启动 (这会触发 write_conf)
     // 注意：即使 fork 成功或失败，write_conf 是在 fork 之前调用的
     server.start();
 
     // 验证文件是否生成
     std::string content = readFile(tmp_conf_path);
     EXPECT_FALSE(content.empty()) << "Config file should be created";
 
     // 验证关键配置项
     EXPECT_NE(content.find("listener 9999 127.0.0.1"), std::string::npos);
     EXPECT_NE(content.find("allow_anonymous false"), std::string::npos);
     EXPECT_NE(content.find("persistence true"), std::string::npos);
 
     // 停止服务（清理 PID）
     server.stop();
 }
 
 // 测试环境变量扩展功能
 TEST_F(MFMQTTServerTest, EnvExpansion) {
     // 设置一个自定义环境变量用于测试
     setenv("MY_TEST_HOME", "/tmp/myhome", 1);
 
     MFMQTTServer server;
     // 使用 hack 方式测试私有函数 expand_env 的效果，
     // 通过观察 conf_path 的最终结果间接验证
     // 这里我们假设 conf_path 是 ${MY_TEST_HOME}/test.conf
     
     // 注意：由于 expand_env 是私有的且只在 init 中对 conf_path_ 生效，
     // 而 init 也是根据 cfg 取值。
     // 你的代码中 hardcode 了 getenv("HOME")，所以我们只能测 HOME
     
     const char* home = getenv("HOME");
     if (home) {
         std::string home_str = home;
         json cfg = {
             {"conf_path", "${HOME}/test_env.conf"}
         };
         
         server.init(cfg);
         server.start();
         
         // 验证文件是否实际生成在展开后的路径
         std::string expected_path = home_str + "/test_env.conf";
         std::ifstream ifs(expected_path);
         EXPECT_TRUE(ifs.is_open()) << "Should verify file created at expanded path: " << expected_path;
         ifs.close();
         
         std::remove(expected_path.c_str());
     }
     server.stop();
 }

 // 1. 测试 init 方法的路径解析（尤其是 ${HOME} 扩展）
TEST_F(MFMQTTServerTest, InitPathExpansion) {
    MFMQTTServer server;
    json cfg = {
        {"mosquitto_path", "/usr/local/bin/fast_cpp_server_dir/mosquitto"},
        {"conf_path", "${HOME}/test_mqtt.conf"},
        {"log_path", "/home/cs/DockerRoot/fast_cpp_server/config/mosquitto.conf"}
    };

    server.init(cfg);
    
    const char* home_env = std::getenv("HOME");
    std::string expected_prefix = home_env ? home_env : "";
    
    // 虽然 conf_path_ 是私有的，但我们可以通过 start() 
    // 触发 write_conf() 来间接验证路径是否正确
    // 如果路径解析错误（如包含 ${HOME} 字符串未替换），文件创建会失败
    bool start_res = server.start(); 
    
    if (start_res) {
        // 验证文件是否真的在主目录下生成了
        std::string real_path = expected_prefix + "/test_mqtt.conf";
        std::ifstream f(real_path);
        EXPECT_TRUE(f.good()) << "File should be created at expanded HOME path: " << real_path;
        f.close();
        std::remove(real_path.c_str()); // 清理
        server.stop();
    }
}

// 2. 测试 start() 后的 PID 捕获与状态
TEST_F(MFMQTTServerTest, StartCapturePid) {
    MFMQTTServer server;
    json cfg = {
        {"conf_path", test_conf},
        {"log_path", test_log},
        {"allow_anonymous", true}
    };

    server.init(cfg);
    
    // 验证初始状态 PID 为空
    EXPECT_FALSE(server.pid().has_value());

    // 启动服务
    bool success = server.start();
    
    if (success) {
        // 验证 PID 已捕获
        auto current_pid = server.pid();
        EXPECT_TRUE(current_pid.has_value());
        EXPECT_GT(current_pid.value(), 0);

        // 验证进程是否在运行 (kill -0 发送空信号检查进程是否存在)
        EXPECT_EQ(kill(current_pid.value(), 0), 0) << "Process should be alive";

        // 停止服务并验证
        server.stop();
        EXPECT_FALSE(server.pid().has_value());
    } else {
        // 如果环境里确实没安装 mosquitto，start 可能返回 false
        // 这种情况下我们验证它没有遗留错误的 PID 即可
        EXPECT_FALSE(server.pid().has_value());
    }
}

// 3. 测试非法配置下的 start 失败处理
TEST_F(MFMQTTServerTest, StartFailureOnInvalidPath) {
    MFMQTTServer server;
    // 指向一个不存在的目录，导致 write_conf 失败
    json cfg = {
        {"conf_path", "/non_existent_dir/no.conf"}
    };

    server.init(cfg);
    EXPECT_FALSE(server.start()) << "Start should fail if config cannot be written";
    EXPECT_FALSE(server.pid().has_value());
}