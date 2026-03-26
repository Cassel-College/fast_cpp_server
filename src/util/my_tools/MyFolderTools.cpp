#include "MyFolderTools.h"

#include "MyLog.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace {

bool RemoveDirectoryRecursivelyOld(const std::string& path)
{
    struct stat st;
    if (::lstat(path.c_str(), &st) != 0) {
        if (errno == ENOENT) {
            return true;
        }
        return false;
    }

    if (!S_ISDIR(st.st_mode)) {
        return ::unlink(path.c_str()) == 0;
    }

    DIR* dir = ::opendir(path.c_str());
    if (dir == nullptr) {
        return false;
    }

    bool ok = true;
    struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        const char* name = entry->d_name;
        if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0) {
            continue;
        }

        const std::string child_path = path + "/" + name;
        if (!RemoveDirectoryRecursivelyOld(child_path)) {
            ok = false;
            break;
        }
    }

    ::closedir(dir);

    if (!ok) {
        return false;
    }

    return ::rmdir(path.c_str()) == 0;
}

std::string BuildTargetPath(const std::string& parent_dir, const std::string& folder_name)
{
    if (parent_dir.empty()) {
        return folder_name;
    }
    if (!parent_dir.empty() && parent_dir.back() == '/') {
        return parent_dir + folder_name;
    }
    return parent_dir + "/" + folder_name;
}

} // namespace

namespace my_tools {

bool MyFolderTools::HasFolderV1(const std::string& parent_dir, const std::string& folder_name)
{
    if (parent_dir.empty() || folder_name.empty()) {
        return false;
    }

    const std::string target_path = parent_dir + "/" + folder_name;

    struct stat st;
    if (::stat(target_path.c_str(), &st) != 0) {
        MYLOG_WARN("目录不存在: {} 于 {}", folder_name, parent_dir);
        return false;
    }

    const bool is_dir = S_ISDIR(st.st_mode);
    if (is_dir) {
        MYLOG_INFO("目录存在: {} 于 {}", folder_name, parent_dir);
    } else {
        MYLOG_WARN("路径存在但不是目录: {} 于 {}", folder_name, parent_dir);
    }

    return is_dir;
}

bool MyFolderTools::HasFolderV2(const std::string& parent_dir, const std::string& folder_name)
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
bool MyFolderTools::DeleteFolderByPathV1(const std::string& full_path)
{
    if (full_path.empty()) {
        MYLOG_WARN("DeleteFolderByPathV1 参数非法: full_path 为空");
        return false;
    }

    struct stat st;
    if (::lstat(full_path.c_str(), &st) != 0) {
        if (errno == ENOENT) {
            MYLOG_INFO("DeleteFolderByPathV1 目录不存在，无需删除: {}", full_path);
            return true;
        }
        MYLOG_ERROR("DeleteFolderByPathV1 检查目录失败: {}, error={}", full_path, std::strerror(errno));
        return false;
    }

    if (!S_ISDIR(st.st_mode)) {
        MYLOG_WARN("DeleteFolderByPathV1 目标存在但不是目录: {}", full_path);
        return false;
    }

    const bool removed = RemoveDirectoryRecursivelyOld(full_path);
    if (removed) {
        MYLOG_INFO("DeleteFolderByPathV1 删除目录成功: {}", full_path);
    } else {
        MYLOG_ERROR("DeleteFolderByPathV1 删除目录失败: {}, error={}", full_path, std::strerror(errno));
    }
    return removed;
}

bool MyFolderTools::DeleteFolderByPathV2(const std::string& full_path)
{
    if (full_path.empty()) {
        MYLOG_WARN("DeleteFolderByPathV2 参数非法: full_path 为空");
        return false;
    }

    const std::filesystem::path target_path(full_path);
    std::error_code ec;

    if (!std::filesystem::exists(target_path, ec)) {
        if (ec) {
            MYLOG_ERROR("DeleteFolderByPathV2 检查目录失败: {}, error={}", target_path.string(), ec.message());
            return false;
        }
        MYLOG_INFO("DeleteFolderByPathV2 目录不存在，无需删除: {}", target_path.string());
        return true;
    }

    if (!std::filesystem::is_directory(target_path, ec)) {
        MYLOG_WARN("DeleteFolderByPathV2 目标存在但不是目录: {}", target_path.string());
        return false;
    }

    const auto removed_count = std::filesystem::remove_all(target_path, ec);
    if (ec) {
        MYLOG_ERROR("DeleteFolderByPathV2 删除目录失败: {}, error={}", target_path.string(), ec.message());
        return false;
    }

    MYLOG_INFO("DeleteFolderByPathV2 删除目录成功: {}, removed_count={}", target_path.string(), removed_count);
    return true;
}

} // namespace my_tools