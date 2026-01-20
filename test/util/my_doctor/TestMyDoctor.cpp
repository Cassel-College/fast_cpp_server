#include "MyDoctor.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace my_doctor;

/**
 * @brief 测试 my_doctor::MyDoctor 单例获取
 */
TEST(MyDoctorTest, GetInstance) {
    auto& instance1 = my_doctor::MyDoctor::GetInstance();
    auto& instance2 = my_doctor::MyDoctor::GetInstance();
    
    // 验证单例：两次获取的是同一个实例
    ASSERT_EQ(&instance1, &instance2);
}

/**
 * @brief 测试 Init 方法
 */
TEST(MyDoctorTest, Init) {
    std::vector<my_doctor::CheckItem> items = {
        {"env", "/usr/bin/env", "any"},
        {"config", "/etc/hosts", "exists"}
    };

    auto& doctor = my_doctor::MyDoctor::GetInstance();
    bool ok = doctor.Init(items);
    
    ASSERT_TRUE(ok);
}

/**
 * @brief 测试 StartAll 方法
 */
TEST(MyDoctorTest, StartAll) {
    std::vector<my_doctor::CheckItem> items = {
        {"memory", "system", "8GB"},
        {"cpu", "system", "4cores"}
    };

    auto& doctor = my_doctor::MyDoctor::GetInstance();
    doctor.Init(items);
    
    // 不应抛出异常
    ASSERT_NO_THROW(doctor.StartAll());
}

/**
 * @brief 测试 ToJson 方法输出格式
 */
TEST(MyDoctorTest, ToJson) {
    std::vector<my_doctor::CheckItem> items = {
        {"env", "/usr/bin/env", "any"},
        {"config", "/etc/hosts", "exists"}
    };

    auto& doctor = my_doctor::MyDoctor::GetInstance();
    doctor.Init(items);
    doctor.StartAll();

    std::string json = doctor.ToJson();
    
    // 验证 JSON 包含必要字段
    ASSERT_NE(json.find("\"results\""), std::string::npos);
    ASSERT_NE(json.find("\"module\":\"env\""), std::string::npos);
    ASSERT_NE(json.find("\"module\":\"config\""), std::string::npos);
    ASSERT_NE(json.find("\"success\""), std::string::npos);
    ASSERT_NE(json.find("\"message\""), std::string::npos);
    ASSERT_NE(json.find("\"detail\""), std::string::npos);
}

/**
 * @brief 测试空检查项列表
 */
TEST(MyDoctorTest, EmptyItems) {
    std::vector<my_doctor::CheckItem> empty_items;

    auto& doctor = my_doctor::MyDoctor::GetInstance();
    bool ok = doctor.Init(empty_items);
    
    ASSERT_TRUE(ok);
    
    doctor.StartAll();
    std::string json = doctor.ToJson();
    
    // 空列表应返回空数组
    ASSERT_NE(json.find("\"results\": []"), std::string::npos);
}

/**
 * @brief 测试多次初始化
 */
TEST(MyDoctorTest, MultipleInit) {
    std::vector<CheckItem> items1 = {
        {"test1", "path1", "v1"}
    };
    
    std::vector<CheckItem> items2 = {
        {"test2", "path2", "v2"}
    };

    auto& doctor = my_doctor::MyDoctor::GetInstance();
    
    // 第一次初始化
    doctor.Init(items1);
    doctor.StartAll();
    std::string json1 = doctor.ToJson();
    ASSERT_NE(json1.find("test1"), std::string::npos);
    
    // 第二次初始化应覆盖
    doctor.Init(items2);
    doctor.StartAll();
    std::string json2 = doctor.ToJson();
    ASSERT_NE(json2.find("test2"), std::string::npos);
    ASSERT_EQ(json2.find("test1"), std::string::npos);
}