#include "MyIPTools.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/types.h>

#include <cstring>
#include <string>

#include "MyLog.h"

namespace my_tools {

std::string MyIPTools::GetLocalIPv4(bool prefer_non_loopback) {
	struct ifaddrs* interfaces = nullptr;
	if (::getifaddrs(&interfaces) != 0) {
		MYLOG_WARN("[MyIPTools] getifaddrs 失败: {}", std::strerror(errno));
		return "127.0.0.1";
	}

	std::string loopback_ip = "127.0.0.1";
	std::string selected_ip;

	for (struct ifaddrs* item = interfaces; item != nullptr; item = item->ifa_next) {
		if (item->ifa_addr == nullptr || item->ifa_addr->sa_family != AF_INET) {
			continue;
		}

		const unsigned int flags = item->ifa_flags;
		if ((flags & IFF_UP) == 0) {
			continue;
		}

		char ip_buffer[INET_ADDRSTRLEN] = {0};
		const auto* addr_in = reinterpret_cast<const struct sockaddr_in*>(item->ifa_addr);
		if (::inet_ntop(AF_INET, &addr_in->sin_addr, ip_buffer, sizeof(ip_buffer)) == nullptr) {
			continue;
		}

		const bool is_loopback = (flags & IFF_LOOPBACK) != 0;
		if (!is_loopback) {
			selected_ip = ip_buffer;
			if (prefer_non_loopback) {
				break;
			}
		} else if (loopback_ip.empty()) {
			loopback_ip = ip_buffer;
		}
	}

	::freeifaddrs(interfaces);

	if (!selected_ip.empty()) {
		return selected_ip;
	}
	if (!loopback_ip.empty()) {
		return loopback_ip;
	}
	return "127.0.0.1";
}

} // namespace my_tools
