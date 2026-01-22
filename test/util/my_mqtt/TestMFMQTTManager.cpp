/**
 * @file TestMFMQTTManager.cpp
 * @brief Unit tests for the MFMQTTManager
 */
 #include <gtest/gtest.h>
 #include <nlohmann/json.hpp>
 #include "fast_mqtt_client/MFMQTTManager.h"
 
 using namespace fast_mqtt;
 using json = nlohmann::json;
 
 class MFMQTTManagerTest : public ::testing::Test {
 protected:
     MFMQTTManager manager;
     json config;
 
     void SetUp() override {
         config = {
             {"host", "127.0.0.1"},
             {"port", 1883}
         };
     }
 };
 
 // 测试添加和获取客户端
 TEST_F(MFMQTTManagerTest, AddAndGetClient) {
     std::string name = "sensor_1";
     
     // 1. 添加客户端
     bool added = manager.add_client(name, config);
     EXPECT_TRUE(added) << "First add should succeed";
 
     // 2. 获取存在的客户端
     auto cli = manager.get(name);
     EXPECT_NE(cli, nullptr) << "Should verify retrieve the added client";
     
     // 3. 获取不存在的客户端
     auto non_exist = manager.get("ghost_client");
     EXPECT_EQ(non_exist, nullptr) << "Should return nullptr for non-existent client";
 }
 
 // 测试禁止重复添加
 TEST_F(MFMQTTManagerTest, PreventDuplicateAdd) {
     std::string name = "sensor_duplicate";
     
     EXPECT_TRUE(manager.add_client(name, config));
     
     // 再次添加同名客户端应失败
     EXPECT_FALSE(manager.add_client(name, config)) << "Adding duplicate client name should fail";
 }
 
 // 测试删除客户端
 TEST_F(MFMQTTManagerTest, RemoveClient) {
     std::string name = "sensor_removable";
     manager.add_client(name, config);
     
     // 1. 正常删除
     EXPECT_TRUE(manager.remove_client(name));
     
     // 2. 验证已删除
     EXPECT_EQ(manager.get(name), nullptr);
     
     // 3. 删除不存在的
     EXPECT_FALSE(manager.remove_client(name)) << "Removing non-existent client should return false";
 }
 
 // 测试通过 Manager 代理发布消息
 TEST_F(MFMQTTManagerTest, ProxyPublish) {
     std::string name = "pub_client";
     manager.add_client(name, config);
 
     // 尝试发布 (由于未连接真实 Broker，可能返回 false，但我们主要测试 Manager 的转发逻辑不崩溃)
     bool result = manager.publish(name, "topic", "msg");
     // 不崩溃即视为通过
     (void)result; 
     
     // 尝试向不存在的客户端发布
     EXPECT_FALSE(manager.publish("unknown", "t", "m"));
 }