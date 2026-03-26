#pragma once

#include <string>

namespace my_tools {

class MyFolderTools {
public:
    /**
     * @brief 判断指定目录下是否存在指定名称的子目录
     * @param parent_dir 待检查的父目录路径
     * @param folder_name 待检查的子目录名称
     * @return true 存在同名子目录，false 不存在或路径非法
     */
    static bool HasFolder(const std::string& parent_dir, const std::string& folder_name);
};

} // namespace my_tools