#include "MyFolderTools.h"

#include "MyLog.h"
#include <filesystem>

namespace my_tools {

bool MyFolderTools::HasFolder(const std::string& parent_dir, const std::string& folder_name)
{

    if (parent_dir.empty() || folder_name.empty()) {
        return false;
    }

    const std::filesystem::path parent_path(parent_dir);
    const std::filesystem::path target_path = parent_path / folder_name;

    bool exists = std::filesystem::exists(target_path) && std::filesystem::is_directory(target_path);
    if (exists) {
        MYLOG_INFO("目录存在: {} 于 {}", target_path.string(), parent_path.string());
    } else {
        MYLOG_WARN("目录不存在: {} 于 {}", target_path.string(), parent_path.string());
    }
    return exists;
}

} // namespace my_tools