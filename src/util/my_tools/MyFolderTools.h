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
    static bool HasFolderV1(const std::string& parent_dir, const std::string& folder_name);

    /**
     * @brief 判断指定目录下是否存在指定名称的子目录
     * @param parent_dir 待检查的父目录路径
     * @param folder_name 待检查的子目录名称
     * @return true 存在同名子目录，false 不存在或路径非法
     */
    static bool HasFolderV2(const std::string& parent_dir, const std::string& folder_name);

    /**
     * @brief 删除指定路径的目录（如果存在）
     * @param full_path 待删除目录的完整路径
     * @return true 删除成功或目录不存在，false 删除失败
     */
    static bool DeleteFolderByPathV1(const std::string& full_path);

    /**
     * @brief 删除指定路径的目录（如果存在）
     * @param full_path 待删除目录的完整路径
     * @return true 删除成功或目录不存在，false 删除失败
     */
    static bool DeleteFolderByPathV2(const std::string& full_path);

};

} // namespace my_tools