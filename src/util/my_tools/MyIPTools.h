#pragma once

#include <string>

namespace my_tools {

class MyIPTools {
public:
	/**
	 * @brief 获取本机 IPv4 地址
	 * @param prefer_non_loopback 是否优先返回非回环地址
	 * @return 找到的 IPv4 地址；若未找到有效地址则回退为 127.0.0.1
	 */
	static std::string GetLocalIPv4(bool prefer_non_loopback = true);
};

} // namespace my_tools
