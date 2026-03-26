#include "MyFileTools.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "MyLog.h"

namespace {

bool WriteTextInternal(const std::string& file_path, const std::string& content, int flags, const char* mode_name) {
	if (file_path.empty()) {
		MYLOG_WARN("MyFileTools::{} 失败: file_path 为空", mode_name);
		return false;
	}

	const int fd = ::open(file_path.c_str(), flags, 0644);
	if (fd < 0) {
		MYLOG_ERROR("MyFileTools::{} 打开文件失败: {}, error={}", mode_name, file_path, std::strerror(errno));
		return false;
	}

	const char* data = content.data();
	size_t total_written = 0;
	const size_t total_size = content.size();
	while (total_written < total_size) {
		const ssize_t written = ::write(fd, data + total_written, total_size - total_written);
		if (written < 0) {
			MYLOG_ERROR("MyFileTools::{} 写入文件失败: {}, error={}", mode_name, file_path, std::strerror(errno));
			::close(fd);
			return false;
		}
		total_written += static_cast<size_t>(written);
	}

	if (::close(fd) != 0) {
		MYLOG_ERROR("MyFileTools::{} 关闭文件失败: {}, error={}", mode_name, file_path, std::strerror(errno));
		return false;
	}

	MYLOG_INFO("MyFileTools::{} 成功: {}, bytes={}", mode_name, file_path, total_size);
	return true;
}

} // namespace

namespace my_tools {

bool MyFileTools::Exists(const std::string& file_path) {
	if (file_path.empty()) {
		MYLOG_WARN("MyFileTools::Exists 失败: file_path 为空");
		return false;
	}

	struct stat st;
	if (::stat(file_path.c_str(), &st) != 0) {
		if (errno != ENOENT) {
			MYLOG_WARN("MyFileTools::Exists 检查失败: {}, error={}", file_path, std::strerror(errno));
		}
		return false;
	}

	const bool is_regular = S_ISREG(st.st_mode);
	if (!is_regular) {
		MYLOG_WARN("MyFileTools::Exists 路径存在但不是普通文件: {}", file_path);
	}
	return is_regular;
}

bool MyFileTools::CreateFile(const std::string& file_path, bool truncate_if_exists) {
	if (file_path.empty()) {
		MYLOG_WARN("MyFileTools::CreateFile 失败: file_path 为空");
		return false;
	}

	const int flags = truncate_if_exists ? (O_WRONLY | O_CREAT | O_TRUNC) : (O_WRONLY | O_CREAT);
	const int fd = ::open(file_path.c_str(), flags, 0644);
	if (fd < 0) {
		MYLOG_ERROR("MyFileTools::CreateFile 创建失败: {}, error={}", file_path, std::strerror(errno));
		return false;
	}

	if (::close(fd) != 0) {
		MYLOG_ERROR("MyFileTools::CreateFile 关闭失败: {}, error={}", file_path, std::strerror(errno));
		return false;
	}

	MYLOG_INFO("MyFileTools::CreateFile 成功: {}, truncate_if_exists={}", file_path, truncate_if_exists);
	return true;
}

bool MyFileTools::DeleteFile(const std::string& file_path) {
	if (file_path.empty()) {
		MYLOG_WARN("MyFileTools::DeleteFile 失败: file_path 为空");
		return false;
	}

	struct stat st;
	if (::lstat(file_path.c_str(), &st) != 0) {
		if (errno == ENOENT) {
			MYLOG_INFO("MyFileTools::DeleteFile 文件不存在，无需删除: {}", file_path);
			return true;
		}
		MYLOG_ERROR("MyFileTools::DeleteFile 检查失败: {}, error={}", file_path, std::strerror(errno));
		return false;
	}

	if (!S_ISREG(st.st_mode)) {
		MYLOG_WARN("MyFileTools::DeleteFile 目标不是普通文件: {}", file_path);
		return false;
	}

	if (::unlink(file_path.c_str()) != 0) {
		MYLOG_ERROR("MyFileTools::DeleteFile 删除失败: {}, error={}", file_path, std::strerror(errno));
		return false;
	}

	MYLOG_INFO("MyFileTools::DeleteFile 删除成功: {}", file_path);
	return true;
}

bool MyFileTools::AppendText(const std::string& file_path, const std::string& content) {
	return WriteTextInternal(file_path, content, O_WRONLY | O_CREAT | O_APPEND, "AppendText");
}

bool MyFileTools::WriteText(const std::string& file_path, const std::string& content) {
	return WriteTextInternal(file_path, content, O_WRONLY | O_CREAT | O_TRUNC, "WriteText");
}

} // namespace my_tools
