#pragma once

#include <string>

namespace my_tools {

class MyFileTools {
public:
	/**
	 * @brief 判断指定文件是否存在且为普通文件
	 * @param file_path 文件路径
	 * @return true 文件存在且是普通文件；false 不存在或不是普通文件
	 */
	static bool Exists(const std::string& file_path);

	/**
	 * @brief 创建指定文件
	 * @param file_path 文件路径
	 * @param truncate_if_exists 若文件已存在，是否清空原内容
	 * @return true 成功；false 失败
	 * @note 不负责自动创建父目录；若父目录不存在会返回 false
	 */
	static bool CreateFile(const std::string& file_path, bool truncate_if_exists = false);

	/**
	 * @brief 删除指定文件
	 * @param file_path 文件路径
	 * @return true 删除成功，或文件本来就不存在；false 删除失败
	 */
	static bool DeleteFile(const std::string& file_path);

	/**
	 * @brief 以追加方式写入文本
	 * @param file_path 文件路径
	 * @param content 待写入内容
	 * @return true 写入成功；false 写入失败
	 */
	static bool AppendText(const std::string& file_path, const std::string& content);

	/**
	 * @brief 以覆盖方式写入文本
	 * @param file_path 文件路径
	 * @param content 待写入内容
	 * @return true 写入成功；false 写入失败
	 */
	static bool WriteText(const std::string& file_path, const std::string& content);
};

} // namespace my_tools
