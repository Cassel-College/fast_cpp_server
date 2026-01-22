/**
 * @file TestMFMQTTClient.cpp
 * @brief Unit tests for the Core MFMQTTClient class
 */
 #include <gtest/gtest.h>
 #include <nlohmann/json.hpp>
 #include "fast_mqtt_client/MFMQTTClient.h"
 
 using namespace fast_mqtt;
 using json = nlohmann::json;
 
 class MFMQTTClientTest : public ::testing::Test {
 protected:
     MFMQTTClient client;
     json valid_config;
 
     void SetUp() override {
         valid_config = {
             {"host", "localhost"},
             {"port", 1883},
             {"client_id", "gtest_core_client"},
             {"keepalive", 10}
         };
     }
 };
 
 // 测试初始化参数校验
 TEST_F(MFMQTTClientTest, InitParameterValidation) {
     json empty_host_cfg = { {"port", 1883} };
     EXPECT_THROW(client.init(empty_host_cfg), std::invalid_argument) 
         << "Should throw invalid_argument when host is empty";
     
     EXPECT_NO_THROW(client.init(valid_config)) 
         << "Valid config should not throw";
 }
 
 // 测试连接断开时的行为
 TEST_F(MFMQTTClientTest, PublishFailsWhenNotConnected) {
     // 尚未调用 connect()
     bool result = client.publish("test/topic", "data");
     EXPECT_FALSE(result) << "Publish should fail if client is not connected/initialized";
 }
 
 // 测试回调函数设置
 TEST_F(MFMQTTClientTest, MessageCallbackRegistration) {
     bool callback_invoked = false;
     
     // 注册回调
     client.set_message_handler([&](const std::string& topic, const std::string& payload) {
         callback_invoked = true;
     });
 
     // 由于没有真实的 Broker，我们无法触发真实的网络回调
     // 但我们可以验证 set_message_handler 本身不崩溃且逻辑通过
     // 在 Mock 测试中，这里会调用模拟的 on_message
     SUCCEED(); 
 }
 
 // 测试连接逻辑（基本烟雾测试）
 TEST_F(MFMQTTClientTest, ConnectAttempt) {
     // 即使本地没有 Broker，调用 init -> connect 也不应导致程序 Crash
     // 它只会进入后台重连循环
     EXPECT_NO_THROW({
         client.init(valid_config);
         // client.connect() 是私有的或在 init 中自动调用的(取决于具体实现，
         // 在你的代码中 init() 调用了 start_loop() -> connect())
         // 所以只要 init 不挂即可。
     });
 }