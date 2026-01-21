#include "MyDoctor.h"
#include <nlohmann/json.hpp>

namespace my_doctor {

MyDoctor& MyDoctor::GetInstance() {
    static MyDoctor instance;
    return instance;
}

MyDoctor::MyDoctor() = default;
MyDoctor::~MyDoctor() = default;

bool MyDoctor::InitDefault() {
    std::vector<CheckItem> default_items = {
        {"Git Version Check", CheckType::COMMAND, "Git", "git --version", "2.20.0"},
        {"CMake Version Check", CheckType::COMMAND, "CMake", "cmake --version", "3.15.0"},
        {"Log Directory Check", CheckType::FILE_PATH, "Log Directory", "/var/log/fast_cpp_server/", "exists"},
        {"MySQL Service Check", CheckType::SERVICE, "MySQL Service", "mysql", "running"}
    };
    return Init(default_items);
}

bool MyDoctor::Init(const std::vector<CheckItem>& items) {
    std::lock_guard<std::mutex> lock(mtx_);
    items_ = items;
    results_.clear();
    MYLOG_INFO("* 模块: {}, 状态: {}", "MyDoctor", "已完成初始化");
    return true;
}

void MyDoctor::StartAll() {
    std::lock_guard<std::mutex> lock(mtx_);
    results_.clear();
    for (const auto& item : items_) {
        MYLOG_INFO("* 模块: {}, 状态: {}", item.module_name, "线程已成功创建并加入管理列表");
        auto result = RunCheck(item);
        results_.push_back(result);
        MYLOG_INFO("* 模块: {}, 状态: {}", item.module_name, result.success ? "检查通过" : "检查失败");
    }
}

CheckResult MyDoctor::RunCheck(const CheckItem& item) {
    CheckResult r;
    r.module_name = item.module_name;
    // TODO: 根据 target/expected 实现具体检查逻辑
    r.success = true;
    r.message = "检查通过";
    r.detail  = "占位实现，待补充具体检查逻辑";
    return r;
}

std::string MyDoctor::ToJson() const {
    std::lock_guard<std::mutex> lock(mtx_);
    nlohmann::json json_result;
    for (const auto& r : results_) {
        json_result.push_back({
            {"module", r.module_name},
            {"success", r.success},
            {"message", r.message},
            {"detail", r.detail}
        });
    }
    return json_result.dump();
}

std::string MyDoctor::ShowCheckResults() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string result;
    int index = 0;
    result += "MyDoctor 自检结果:\n";
    result += "======================================\n";
    for (const auto& r : results_) {
        result += "* 模块: " + r.module_name + ", 状态: " + (r.success ? "通过" : "失败") + "\n";
        result += "  - 信息: " + r.message + "\n";
        result += "  - 详情: " + r.detail + "\n";
        index++;
        if (index < results_.size()) {
            result += "--------------------------------------\n";
        }
    }
    result += "======================================\n";
    return result;
}

} // namespace my_docktor