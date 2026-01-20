#pragma once
#ifndef MY_DOCTOR_H
#define MY_DOCTOR_H

#include <string>
#include <vector>
#include <mutex>
#include "MyLog.h"

namespace my_doctor {

/**
 * @brief 单项检查配置
 */
struct CheckItem {
    /// 模块名称
    std::string module_name;
    /// 检查的目标路径或命令
    std::string target;
    /// 期望的版本或阈值
    std::string expected;
};

/**
 * @brief 单项检查结果
 */
struct CheckResult {
    /// 模块名称
    std::string module_name;
    /// 是否通过
    bool success{false};
    /// 结果简述
    std::string message;
    /// 结果详情
    std::string detail;
};

class MyDoctor {
public:
    /**
     * @brief 获取单例实例
     */
    static MyDoctor& GetInstance();

    /**
     * @brief 初始化检查项
     * @param items 需要检查的模块及其参数列表
     * @return 初始化是否成功
     */
    bool Init(const std::vector<CheckItem>& items);

    /**
     * @brief 执行所有检查项
     */
    void StartAll();

    /**
     * @brief 将检查结果转换为 JSON 文本
     * @return JSON 字符串
     */
    std::string ToJson() const;

private:
    MyDoctor();
    ~MyDoctor();

    CheckResult RunCheck(const CheckItem& item);

private:
    std::vector<CheckItem> items_;
    std::vector<CheckResult> results_;
    mutable std::mutex mtx_;
}; // class MyDocter

} // namespace my_docktor

#endif