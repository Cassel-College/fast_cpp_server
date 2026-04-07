#include "MyIPTools.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "MyLog.h"

namespace my_tools {

// ============================================================
//  辅助：错误码转中文
// ============================================================

std::string IPErrorCodeToString(IPErrorCode code) {
	switch (code) {
		case IPErrorCode::kSuccess:           return "操作成功";
		case IPErrorCode::kInvalidCIDR:       return "非法的 CIDR 格式";
		case IPErrorCode::kInterfaceNotFound: return "网卡不存在";
		case IPErrorCode::kPermissionDenied:  return "无 Root 权限，操作被拒绝";
		case IPErrorCode::kSystemError:       return "系统调用失败";
		case IPErrorCode::kIPAlreadyExists:   return "该 IP 已存在于目标网卡";
		case IPErrorCode::kIPNotFound:        return "目标网卡上未找到该 IP";
		default:                              return "未知错误";
	}
}

// ============================================================
//  GetLocalIPv4 —— 保留原有实现
// ============================================================

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

// ============================================================
//  GetAllInterfaces —— 枚举所有 UP 状态的非回环网卡
// ============================================================

std::vector<std::string> MyIPTools::GetAllInterfaces() {
	std::vector<std::string> result;

	struct ifaddrs* interfaces = nullptr;
	if (::getifaddrs(&interfaces) != 0) {
		MYLOG_WARN("[MyIPTools] getifaddrs 失败: {}", std::strerror(errno));
		return result;
	}

	for (struct ifaddrs* item = interfaces; item != nullptr; item = item->ifa_next) {
		if (item->ifa_name == nullptr) continue;
		const unsigned int flags = item->ifa_flags;
		if ((flags & IFF_UP) == 0) continue;
		if ((flags & IFF_LOOPBACK) != 0) continue;

		std::string name(item->ifa_name);
		// 去重
		bool found = false;
		for (const auto& existing : result) {
			if (existing == name) { found = true; break; }
		}
		if (!found) {
			result.push_back(std::move(name));
		}
	}

	::freeifaddrs(interfaces);
	MYLOG_INFO("[MyIPTools] 获取到 {} 个网卡", result.size());
	return result;
}

// ============================================================
//  GetAllIPs —— 枚举所有 IPv4 地址
// ============================================================

std::vector<IPInfo> MyIPTools::GetAllIPs(bool include_loopback) {
	std::vector<IPInfo> result;

	struct ifaddrs* interfaces = nullptr;
	if (::getifaddrs(&interfaces) != 0) {
		MYLOG_WARN("[MyIPTools] getifaddrs 失败: {}", std::strerror(errno));
		return result;
	}

	for (struct ifaddrs* item = interfaces; item != nullptr; item = item->ifa_next) {
		if (item->ifa_addr == nullptr || item->ifa_addr->sa_family != AF_INET) {
			continue;
		}
		const unsigned int flags = item->ifa_flags;
		if ((flags & IFF_UP) == 0) continue;
		if (!include_loopback && (flags & IFF_LOOPBACK) != 0) continue;

		char ip_buffer[INET_ADDRSTRLEN] = {0};
		const auto* addr_in = reinterpret_cast<const struct sockaddr_in*>(item->ifa_addr);
		if (::inet_ntop(AF_INET, &addr_in->sin_addr, ip_buffer, sizeof(ip_buffer)) == nullptr) {
			continue;
		}

		// 获取前缀长度
		int prefix_len = 0;
		if (item->ifa_netmask != nullptr) {
			const auto* mask = reinterpret_cast<const struct sockaddr_in*>(item->ifa_netmask);
			uint32_t mask_val = ntohl(mask->sin_addr.s_addr);
			while (mask_val & 0x80000000) {
				prefix_len++;
				mask_val <<= 1;
			}
		}

		IPInfo info;
		info.interface = item->ifa_name ? item->ifa_name : "";
		info.ip = ip_buffer;
		info.prefix_len = prefix_len;
		result.push_back(std::move(info));
	}

	::freeifaddrs(interfaces);
	MYLOG_INFO("[MyIPTools] 获取到 {} 条 IP 信息", result.size());
	return result;
}

// ============================================================
//  CIDR 校验与解析
// ============================================================

bool MyIPTools::ParseCIDR(const std::string& cidr, std::string& ip, int& prefix_len) {
	auto pos = cidr.find('/');
	if (pos == std::string::npos || pos == 0 || pos == cidr.size() - 1) {
		return false;
	}

	ip = cidr.substr(0, pos);
	std::string prefix_str = cidr.substr(pos + 1);

	// 校验前缀长度为纯数字
	for (char c : prefix_str) {
		if (c < '0' || c > '9') return false;
	}

	prefix_len = std::stoi(prefix_str);
	if (prefix_len < 0 || prefix_len > 32) return false;

	// 校验 IP 格式
	struct in_addr addr{};
	if (::inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
		return false;
	}

	return true;
}

bool MyIPTools::IsValidCIDR(const std::string& cidr) {
	std::string ip;
	int prefix_len = 0;
	return ParseCIDR(cidr, ip, prefix_len);
}

// ============================================================
//  InterfaceExists —— 判断网卡是否存在
// ============================================================

bool MyIPTools::InterfaceExists(const std::string& interface_name) {
	unsigned int idx = ::if_nametoindex(interface_name.c_str());
	return idx != 0;
}

// ============================================================
//  AddIP / DeleteIP —— 外层接口
// ============================================================

IPResult MyIPTools::AddIP(const std::string& interface_name, const std::string& cidr) {
	MYLOG_INFO("[MyIPTools] 开始向网卡 {} 添加 IP {}", interface_name, cidr);

	std::string ip;
	int prefix_len = 0;
	if (!ParseCIDR(cidr, ip, prefix_len)) {
		MYLOG_WARN("[MyIPTools] CIDR 格式非法: {}", cidr);
		return {IPErrorCode::kInvalidCIDR, "非法的 CIDR 格式: " + cidr};
	}

	if (!InterfaceExists(interface_name)) {
		MYLOG_WARN("[MyIPTools] 网卡不存在: {}", interface_name);
		return {IPErrorCode::kInterfaceNotFound, "网卡不存在: " + interface_name};
	}

	return ModifyIP(interface_name, ip, prefix_len, true);
}

IPResult MyIPTools::DeleteIP(const std::string& interface_name, const std::string& cidr) {
	MYLOG_INFO("[MyIPTools] 开始从网卡 {} 删除 IP {}", interface_name, cidr);

	std::string ip;
	int prefix_len = 0;
	if (!ParseCIDR(cidr, ip, prefix_len)) {
		MYLOG_WARN("[MyIPTools] CIDR 格式非法: {}", cidr);
		return {IPErrorCode::kInvalidCIDR, "非法的 CIDR 格式: " + cidr};
	}

	if (!InterfaceExists(interface_name)) {
		MYLOG_WARN("[MyIPTools] 网卡不存在: {}", interface_name);
		return {IPErrorCode::kInterfaceNotFound, "网卡不存在: " + interface_name};
	}

	return ModifyIP(interface_name, ip, prefix_len, false);
}

// ============================================================
//  ModifyIP —— 通过 rtnetlink 执行添加/删除
// ============================================================

IPResult MyIPTools::ModifyIP(const std::string& interface_name,
                             const std::string& ip, int prefix_len, bool is_add) {
	// 获取网卡索引
	unsigned int if_index = ::if_nametoindex(interface_name.c_str());
	if (if_index == 0) {
		return {IPErrorCode::kInterfaceNotFound, "if_nametoindex 失败: " + interface_name};
	}

	// 解析 IP 地址为二进制
	struct in_addr addr{};
	if (::inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
		return {IPErrorCode::kInvalidCIDR, "inet_pton 解析失败: " + ip};
	}

	// 打开 netlink socket
	int fd = ::socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (fd < 0) {
		if (errno == EPERM || errno == EACCES) {
			MYLOG_ERROR("[MyIPTools] 无 Root 权限，无法创建 netlink socket");
			return {IPErrorCode::kPermissionDenied, "无 Root 权限，无法创建 netlink socket"};
		}
		MYLOG_ERROR("[MyIPTools] 创建 netlink socket 失败: {}", std::strerror(errno));
		return {IPErrorCode::kSystemError, std::string("创建 netlink socket 失败: ") + std::strerror(errno)};
	}

	// 绑定 netlink socket
	struct sockaddr_nl sa{};
	sa.nl_family = AF_NETLINK;
	sa.nl_pid = 0;  // 由内核分配
	if (::bind(fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa)) < 0) {
		::close(fd);
		MYLOG_ERROR("[MyIPTools] 绑定 netlink socket 失败: {}", std::strerror(errno));
		return {IPErrorCode::kSystemError, std::string("绑定 netlink socket 失败: ") + std::strerror(errno)};
	}

	// 构建 rtnetlink 请求
	struct {
		struct nlmsghdr  nlh;
		struct ifaddrmsg ifa;
		char             buf[256];
	} req{};

	req.nlh.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.nlh.nlmsg_type  = is_add ? RTM_NEWADDR : RTM_DELADDR;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	if (is_add) {
		req.nlh.nlmsg_flags |= NLM_F_CREATE | NLM_F_EXCL;
	}
	req.nlh.nlmsg_seq = 1;

	req.ifa.ifa_family    = AF_INET;
	req.ifa.ifa_prefixlen = static_cast<unsigned char>(prefix_len);
	req.ifa.ifa_index     = static_cast<int>(if_index);
	req.ifa.ifa_scope     = RT_SCOPE_UNIVERSE;

	// 添加 IFA_LOCAL 属性
	struct rtattr* rta = reinterpret_cast<struct rtattr*>(
		reinterpret_cast<char*>(&req) + NLMSG_ALIGN(req.nlh.nlmsg_len));
	rta->rta_type = IFA_LOCAL;
	rta->rta_len  = RTA_LENGTH(sizeof(struct in_addr));
	std::memcpy(RTA_DATA(rta), &addr, sizeof(struct in_addr));
	req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + rta->rta_len;

	// 添加 IFA_ADDRESS 属性
	rta = reinterpret_cast<struct rtattr*>(
		reinterpret_cast<char*>(&req) + NLMSG_ALIGN(req.nlh.nlmsg_len));
	rta->rta_type = IFA_ADDRESS;
	rta->rta_len  = RTA_LENGTH(sizeof(struct in_addr));
	std::memcpy(RTA_DATA(rta), &addr, sizeof(struct in_addr));
	req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + rta->rta_len;

	// 发送请求
	if (::send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
		::close(fd);
		if (errno == EPERM || errno == EACCES) {
			MYLOG_ERROR("[MyIPTools] 无 Root 权限，发送 netlink 请求被拒绝");
			return {IPErrorCode::kPermissionDenied, "无 Root 权限，发送 netlink 请求被拒绝"};
		}
		MYLOG_ERROR("[MyIPTools] 发送 netlink 请求失败: {}", std::strerror(errno));
		return {IPErrorCode::kSystemError, std::string("发送 netlink 请求失败: ") + std::strerror(errno)};
	}

	// 接收 ACK 响应
	char resp_buf[4096] = {0};
	ssize_t len = ::recv(fd, resp_buf, sizeof(resp_buf), 0);
	::close(fd);

	if (len < 0) {
		MYLOG_ERROR("[MyIPTools] 接收 netlink 响应失败: {}", std::strerror(errno));
		return {IPErrorCode::kSystemError, std::string("接收 netlink 响应失败: ") + std::strerror(errno)};
	}

	// 解析响应
	auto* nlh = reinterpret_cast<struct nlmsghdr*>(resp_buf);
	if (nlh->nlmsg_type == NLMSG_ERROR) {
		auto* err = reinterpret_cast<struct nlmsgerr*>(NLMSG_DATA(nlh));
		if (err->error == 0) {
			const char* action = is_add ? "添加" : "删除";
			MYLOG_INFO("[MyIPTools] 成功{}网卡 {} 的 IP {}/{}", action, interface_name, ip, prefix_len);
			return {IPErrorCode::kSuccess, std::string("成功") + action + " IP: " + ip + "/" + std::to_string(prefix_len)};
		}

		int abs_err = -err->error;
		if (abs_err == EEXIST) {
			MYLOG_WARN("[MyIPTools] IP {}/{} 已存在于网卡 {}", ip, prefix_len, interface_name);
			return {IPErrorCode::kIPAlreadyExists, "该 IP 已存在于网卡 " + interface_name};
		}
		if (abs_err == EADDRNOTAVAIL) {
			MYLOG_WARN("[MyIPTools] 网卡 {} 上未找到 IP {}/{}", interface_name, ip, prefix_len);
			return {IPErrorCode::kIPNotFound, "网卡 " + interface_name + " 上未找到该 IP"};
		}
		if (abs_err == EPERM || abs_err == EACCES) {
			MYLOG_ERROR("[MyIPTools] 无 Root 权限执行 IP 操作");
			return {IPErrorCode::kPermissionDenied, "无 Root 权限，操作被拒绝"};
		}

		MYLOG_ERROR("[MyIPTools] netlink 操作失败，错误码: {} ({})", abs_err, std::strerror(abs_err));
		return {IPErrorCode::kSystemError, std::string("netlink 操作失败: ") + std::strerror(abs_err)};
	}

	return {IPErrorCode::kSuccess, "操作完成"};
}

// ============================================================
//  静态成员初始化
// ============================================================

std::atomic<int> MyIPTools::s_max_threads{64};
std::atomic<int> MyIPTools::s_ping_timeout_ms{800};

// ============================================================
//  InitScanner —— 设置扫描参数
// ============================================================

void MyIPTools::InitScanner(int max_threads, int timeout_ms) {
	if (max_threads < 1) max_threads = 1;
	if (max_threads > 1024) max_threads = 1024;
	if (timeout_ms < 100) timeout_ms = 100;
	if (timeout_ms > 5000) timeout_ms = 5000;

	s_max_threads.store(max_threads);
	s_ping_timeout_ms.store(timeout_ms);
	MYLOG_INFO("[MyIPTools] 扫描器初始化: 最大线程数={}, 超时={}ms", max_threads, timeout_ms);
}

// ============================================================
//  HasRawSocketPermission —— 检测 CAP_NET_RAW / Root
// ============================================================

bool MyIPTools::HasRawSocketPermission() {
	// 方式一：Root 用户
	if (::geteuid() == 0) {
		return true;
	}

	// 方式二：尝试创建 raw socket 检测 CAP_NET_RAW
	int fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd >= 0) {
		::close(fd);
		return true;
	}
	return false;
}

// ============================================================
//  ComputeICMPChecksum —— RFC 1071 校验和
// ============================================================

uint16_t MyIPTools::ComputeICMPChecksum(const void* data, int len) {
	const auto* ptr = static_cast<const uint16_t*>(data);
	uint32_t sum = 0;

	while (len > 1) {
		sum += *ptr++;
		len -= 2;
	}

	// 剩余奇数字节
	if (len == 1) {
		uint16_t last = 0;
		std::memcpy(&last, ptr, 1);
		sum += last;
	}

	// 折叠进位
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return static_cast<uint16_t>(~sum);
}

// 全局原子序列号，保证每次 PingHost 调用使用唯一 sequence
static std::atomic<uint16_t> s_ping_sequence{0};

// ============================================================
//  PingHost —— 原生 ICMP Echo Request/Reply
// ============================================================

bool MyIPTools::PingHost(const std::string& ip, int timeout_ms) {
	// 创建 raw socket
	int fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd < 0) {
		return false;
	}

	// 设置接收超时（用于每次 recvfrom 单次阻塞上限）
	struct timeval tv{};
	tv.tv_sec  = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;
	::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	// 目标地址
	struct sockaddr_in dest{};
	dest.sin_family = AF_INET;
	if (::inet_pton(AF_INET, ip.c_str(), &dest.sin_addr) != 1) {
		::close(fd);
		return false;
	}

	// 构建 ICMP Echo Request —— 使用唯一 sequence 区分并发请求
	const uint16_t my_seq = s_ping_sequence.fetch_add(1);
	ICMPHeader icmp_req{};
	icmp_req.type     = 8;  // Echo Request
	icmp_req.code     = 0;
	icmp_req.checksum = 0;
	icmp_req.id       = static_cast<uint16_t>(::getpid() & 0xFFFF);
	icmp_req.sequence = my_seq;
	icmp_req.checksum = ComputeICMPChecksum(&icmp_req, sizeof(icmp_req));

	// 发送
	ssize_t sent = ::sendto(fd, &icmp_req, sizeof(icmp_req), 0,
	                        reinterpret_cast<struct sockaddr*>(&dest), sizeof(dest));
	if (sent < 0) {
		::close(fd);
		return false;
	}

	// 接收应答 —— 循环读取，丢弃不匹配的包（其他线程的 reply），
	// 直到收到来源 IP 匹配的 Echo Reply 或超时
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

	while (true) {
		char recv_buf[256] = {0};
		struct sockaddr_in from{};
		socklen_t from_len = sizeof(from);
		ssize_t received = ::recvfrom(fd, recv_buf, sizeof(recv_buf), 0,
		                              reinterpret_cast<struct sockaddr*>(&from), &from_len);

		if (received < 0) {
			::close(fd);
			return false;  // 超时或错误
		}

		// 检查是否已超过总截止时间
		if (std::chrono::steady_clock::now() >= deadline) {
			break;
		}

		// 解析 IP 头部 + ICMP 头部
		if (received < 20 + static_cast<ssize_t>(sizeof(ICMPHeader))) {
			continue;  // 包太短，继续等待
		}

		int ip_hdr_len = (static_cast<uint8_t>(recv_buf[0]) & 0x0F) * 4;
		if (received < ip_hdr_len + static_cast<ssize_t>(sizeof(ICMPHeader))) {
			continue;
		}

		const auto* reply = reinterpret_cast<const ICMPHeader*>(recv_buf + ip_hdr_len);

		// 严格三重校验：
		//   1. type=0 Echo Reply
		//   2. ID 匹配（本进程发出）
		//   3. 来源 IP == 目标 IP（关键：排除其他线程收到的串扰回复）
		if (reply->type == 0
		    && reply->id == icmp_req.id
		    && from.sin_addr.s_addr == dest.sin_addr.s_addr) {
			::close(fd);
			return true;
		}
		// 不匹配 → 继续循环等待正确的应答
	}

	::close(fd);
	return false;
}

// ============================================================
//  GenerateSubnetIPs —— 计算网段内候选 IP
// ============================================================

std::vector<std::string> MyIPTools::GenerateSubnetIPs(const std::string& ip, int prefix_len) {
	std::vector<std::string> result;

	// 限制扫描范围：前缀 < 20 时主机数超 4094，扫描代价过大
	if (prefix_len < 20 || prefix_len > 30) {
		MYLOG_WARN("[MyIPTools] 前缀长度 {} 不在可扫描范围 [20,30] 内，跳过（/16~/19 网段过大）", prefix_len);
		return result;
	}

	struct in_addr addr{};
	if (::inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
		return result;
	}

	uint32_t ip_host   = ntohl(addr.s_addr);
	uint32_t mask      = prefix_len == 0 ? 0 : (~uint32_t(0)) << (32 - prefix_len);
	uint32_t network   = ip_host & mask;
	uint32_t broadcast = network | (~mask);

	// 排除网络地址和广播地址
	uint32_t first = network + 1;
	uint32_t last  = broadcast - 1;

	if (first > last) {
		return result;
	}

	result.reserve(static_cast<size_t>(last - first + 1));
	char buf[INET_ADDRSTRLEN] = {0};
	for (uint32_t h = first; h <= last; ++h) {
		struct in_addr a{};
		a.s_addr = htonl(h);
		if (::inet_ntop(AF_INET, &a, buf, sizeof(buf)) != nullptr) {
			result.emplace_back(buf);
		}
	}

	return result;
}

// ============================================================
//  ScanActiveDevices —— 全局扫描入口
// ============================================================

ScanResult MyIPTools::ScanActiveDevices() {
	ScanResult scan_result;

	auto start_time = std::chrono::steady_clock::now();

	// 权限检查
	if (!HasRawSocketPermission()) {
		MYLOG_ERROR("[MyIPTools] 无 CAP_NET_RAW 权限或非 Root 用户，无法执行 ICMP 扫描");
		scan_result.error = "无 CAP_NET_RAW 权限或非 Root 用户，无法创建原始套接字执行 ICMP 扫描";
		return scan_result;
	}

	// 获取所有本机 IP
	auto local_ips = GetAllIPs(false);
	if (local_ips.empty()) {
		MYLOG_WARN("[MyIPTools] 未获取到任何本机 IP，无法执行扫描");
		scan_result.error = "未获取到任何本机非回环 IP 地址";
		return scan_result;
	}

	// 收集所有本机 IP 用于去重排除
	std::set<std::string> self_ips;
	for (const auto& info : local_ips) {
		self_ips.insert(info.ip);
	}

	// 跳过虚拟网卡（Docker / bridge / veth / virbr 等），避免扫描无意义的大网段
	auto is_virtual_iface = [](const std::string& iface) -> bool {
		if (iface == "docker0")                      return true;
		if (iface.rfind("br-", 0) == 0)              return true;
		if (iface.rfind("veth", 0) == 0)             return true;
		if (iface.rfind("virbr", 0) == 0)            return true;
		if (iface.rfind("cni", 0) == 0)              return true;
		if (iface.rfind("flannel", 0) == 0)          return true;
		return false;
	};

	// 生成所有候选 IP（去重：多网卡可能有重叠网段）
	std::set<std::string> candidate_set;
	for (const auto& info : local_ips) {
		if (is_virtual_iface(info.interface)) {
			MYLOG_INFO("[MyIPTools] 跳过虚拟网卡: {} ({}/{})", info.interface, info.ip, info.prefix_len);
			continue;
		}
		MYLOG_INFO("[MyIPTools] 解析网段: {}/{} (网卡: {})", info.ip, info.prefix_len, info.interface);
		auto subnet_ips = GenerateSubnetIPs(info.ip, info.prefix_len);
		for (auto& sip : subnet_ips) {
			// 排除本机地址
			if (self_ips.count(sip) == 0) {
				candidate_set.insert(std::move(sip));
			}
		}
	}

	std::vector<std::string> candidates(candidate_set.begin(), candidate_set.end());
	scan_result.total_scanned = static_cast<int>(candidates.size());
	MYLOG_INFO("[MyIPTools] 待扫描 IP 总数: {}", candidates.size());

	if (candidates.empty()) {
		auto end_time = std::chrono::steady_clock::now();
		scan_result.scan_duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
		return scan_result;
	}

	// 并发扫描：使用条件变量控制最大线程数（C++17 兼容）
	const int max_threads = s_max_threads.load();
	const int timeout = s_ping_timeout_ms.load();

	std::mutex result_mutex;
	std::vector<std::string> active_ips;

	// C++17 信号量替代：mutex + condition_variable + counter
	std::mutex sem_mutex;
	std::condition_variable sem_cv;
	int sem_count = max_threads;

	auto sem_acquire = [&]() {
		std::unique_lock<std::mutex> lock(sem_mutex);
		sem_cv.wait(lock, [&]() { return sem_count > 0; });
		--sem_count;
	};

	auto sem_release = [&]() {
		{
			std::lock_guard<std::mutex> lock(sem_mutex);
			++sem_count;
		}
		sem_cv.notify_one();
	};

	std::vector<std::thread> threads;
	threads.reserve(candidates.size());

	std::atomic<int> progress{0};
	const int total = static_cast<int>(candidates.size());

	for (const auto& target_ip : candidates) {
		sem_acquire();  // 获取信号量，控制并发数

		threads.emplace_back([&, target_ip]() {
			MYLOG_DEBUG("[MyIPTools] 正在探测 {}...", target_ip);

			bool alive = PingHost(target_ip, timeout);

			if (alive) {
				std::lock_guard<std::mutex> lock(result_mutex);
				active_ips.push_back(target_ip);
				MYLOG_INFO("[MyIPTools] 发现活跃设备: {}", target_ip);
			}

			int done = progress.fetch_add(1) + 1;
			if (done % 50 == 0 || done == total) {
				MYLOG_INFO("[MyIPTools] 扫描进度: {}/{}", done, total);
			}

			sem_release();  // 释放信号量
		});
	}

	// 等待所有线程完成
	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	// 排序结果
	std::sort(active_ips.begin(), active_ips.end(), [](const std::string& a, const std::string& b) {
		struct in_addr aa{}, bb{};
		::inet_pton(AF_INET, a.c_str(), &aa);
		::inet_pton(AF_INET, b.c_str(), &bb);
		return ntohl(aa.s_addr) < ntohl(bb.s_addr);
	});

	auto end_time = std::chrono::steady_clock::now();

	scan_result.active_ips = std::move(active_ips);
	scan_result.active_count = static_cast<int>(scan_result.active_ips.size());
	scan_result.scan_duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

	MYLOG_INFO("[MyIPTools] 扫描完成: 总计扫描 {} 个 IP, 发现 {} 个活跃设备, 耗时 {:.1f}ms",
	           scan_result.total_scanned, scan_result.active_count, scan_result.scan_duration_ms);

	return scan_result;
}

// ============================================================
//  ScanTargetSubnet —— 扫描指定网段
// ============================================================

ScanResult MyIPTools::ScanTargetSubnet(const std::string& cidr) {
	ScanResult scan_result;
	auto start_time = std::chrono::steady_clock::now();

	// 解析 CIDR
	std::string ip;
	int prefix_len = 0;
	if (!ParseCIDR(cidr, ip, prefix_len)) {
		scan_result.error = "CIDR 格式非法: " + cidr;
		MYLOG_WARN("[MyIPTools] 指定网段扫描失败: {}", scan_result.error);
		return scan_result;
	}

	// 权限检查
	if (!HasRawSocketPermission()) {
		MYLOG_ERROR("[MyIPTools] 无 CAP_NET_RAW 权限或非 Root 用户，无法执行 ICMP 扫描");
		scan_result.error = "无 CAP_NET_RAW 权限或非 Root 用户，无法创建原始套接字执行 ICMP 扫描";
		return scan_result;
	}

	// 指定网段扫描放宽前缀限制到 /20（与 GenerateSubnetIPs 一致）
	auto candidates = GenerateSubnetIPs(ip, prefix_len);
	if (candidates.empty()) {
		scan_result.error = "网段 " + cidr + " 无可扫描的候选 IP（前缀长度需在 20~30 范围内）";
		MYLOG_WARN("[MyIPTools] {}", scan_result.error);
		return scan_result;
	}

	scan_result.total_scanned = static_cast<int>(candidates.size());
	MYLOG_INFO("[MyIPTools] 指定网段扫描: {}，待扫描 IP 总数: {}", cidr, candidates.size());

	// 并发扫描（复用同样的信号量并发模型）
	const int max_threads = s_max_threads.load();
	const int timeout = s_ping_timeout_ms.load();

	std::mutex result_mutex;
	std::vector<std::string> active_ips;

	std::mutex sem_mutex;
	std::condition_variable sem_cv;
	int sem_count = max_threads;

	auto sem_acquire = [&]() {
		std::unique_lock<std::mutex> lock(sem_mutex);
		sem_cv.wait(lock, [&]() { return sem_count > 0; });
		--sem_count;
	};

	auto sem_release = [&]() {
		{
			std::lock_guard<std::mutex> lock(sem_mutex);
			++sem_count;
		}
		sem_cv.notify_one();
	};

	std::vector<std::thread> threads;
	threads.reserve(candidates.size());

	std::atomic<int> progress{0};
	const int total = static_cast<int>(candidates.size());

	for (const auto& target_ip : candidates) {
		sem_acquire();

		threads.emplace_back([&, target_ip]() {
			bool alive = PingHost(target_ip, timeout);

			if (alive) {
				std::lock_guard<std::mutex> lock(result_mutex);
				active_ips.push_back(target_ip);
				MYLOG_INFO("[MyIPTools] 发现活跃设备: {}", target_ip);
			}

			int done = progress.fetch_add(1) + 1;
			if (done % 50 == 0 || done == total) {
				MYLOG_INFO("[MyIPTools] 指定网段扫描进度: {}/{}", done, total);
			}

			sem_release();
		});
	}

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	// IP 排序
	std::sort(active_ips.begin(), active_ips.end(), [](const std::string& a, const std::string& b) {
		struct in_addr aa{}, bb{};
		::inet_pton(AF_INET, a.c_str(), &aa);
		::inet_pton(AF_INET, b.c_str(), &bb);
		return ntohl(aa.s_addr) < ntohl(bb.s_addr);
	});

	auto end_time = std::chrono::steady_clock::now();

	scan_result.active_ips = std::move(active_ips);
	scan_result.active_count = static_cast<int>(scan_result.active_ips.size());
	scan_result.scan_duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

	MYLOG_INFO("[MyIPTools] 指定网段扫描完成 [{}]: 扫描 {} 个 IP, 发现 {} 个活跃设备, 耗时 {:.1f}ms",
	           cidr, scan_result.total_scanned, scan_result.active_count, scan_result.scan_duration_ms);

	return scan_result;
}

} // namespace my_tools
