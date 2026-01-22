// #include <gtest/gtest.h>
// #include <nlohmann/json.hpp>
// #include "fast_mqtt_client/MFSMQTTClient.h"

// using fast_mqtt::MFSMQTTClient;
// using nlohmann::json;

// // 单例应返回同一实例地址
// TEST(MFSMQTTClientTest, SingletonStable) {
//     auto& a = MFSMQTTClient::instance();
//     auto& b = MFSMQTTClient::instance();
//     EXPECT_EQ(&a, &b);
// }

// // 未初始化前 publish/subscribe 返回 false；set_message_handler 可安全调用
// TEST(MFSMQTTClientTest, MethodsBeforeInit) {
//     auto& s = MFSMQTTClient::instance();

//     EXPECT_FALSE(s.publish("topic/a", "payload"));
//     EXPECT_FALSE(s.subscribe("topic/a"));

//     EXPECT_NO_THROW({
//         s.set_message_handler([](const std::string&, const std::string&) {});
//     });
// }

// // 缺失 host 的配置应在 init 时抛出异常
// TEST(MFSMQTTClientTest, InitThrowsOnMissingHost) {
//     auto& s = MFSMQTTClient::instance();
//     json bad_cfg = json::object(); // 未提供 host
//     EXPECT_THROW(s.init(bad_cfg), std::exception);
// }


/**
 * @file TestMFSMQTTClient.cpp
 * @brief Unit tests for the Singleton MQTT Client wrapper
 */
 #include <gtest/gtest.h>
 #include <nlohmann/json.hpp>
 #include "fast_mqtt_client/MFSMQTTClient.h"
 
 using namespace fast_mqtt;
 using json = nlohmann::json;
 
 // 测试单例的唯一性
 TEST(MFSMQTTClientTest, SingletonIdentity) {
     auto& instance1 = MFSMQTTClient::instance();
     auto& instance2 = MFSMQTTClient::instance();
     EXPECT_EQ(&instance1, &instance2) << "MFSMQTTClient::instance() should return the same address";
 }
 
 // 测试未初始化前的防御性编程
 TEST(MFSMQTTClientTest, MethodsSafeBeforeInit) {
     auto& client = MFSMQTTClient::instance();
     
     // 未 Init 前调用业务方法应安全返回 false
     EXPECT_FALSE(client.publish("test/topic", "payload"));
     EXPECT_FALSE(client.subscribe("test/topic"));
 
     // 设置回调不应崩溃
     EXPECT_NO_THROW({
         client.set_message_handler([](const std::string&, const std::string&){});
     });
 }
 
 // 测试配置校验逻辑
 TEST(MFSMQTTClientTest, InitValidation) {
     auto& client = MFSMQTTClient::instance();
 
     // Case 1: 缺少 host 字段
     json bad_cfg = {
         {"port", 1883},
         {"client_id", "test_id"}
     };
     // 预期抛出 invalid_argument 或 runtime_error
     EXPECT_THROW(client.init(bad_cfg), std::exception) << "Should throw when host is missing";
 
     // Case 2: 正常初始化 (注意：单例一旦初始化，后续测试会受影响，
     // 但由于无法重置单例，这里主要测试不抛出异常)
     json good_cfg = {
         {"host", "127.0.0.1"},
         {"port", 1883},
         {"client_id", "gtest_singleton"}
     };
     EXPECT_NO_THROW(client.init(good_cfg));
 }