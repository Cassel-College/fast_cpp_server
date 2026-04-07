#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace my_tools {

/**
 * @brief IP 管理操作的错误码枚举
 */
enum class IPErrorCode {
	kSuccess          = 0,     ///< 操作成功
	kInvalidCIDR      = 1,     ///< 非法的 CIDR 格式（如 "abc/99"）
	kInterfaceNotFound = 2,    ///< 指定的网卡不存在
	kPermissionDenied = 3,     ///< 无 Root 权限，操作被拒绝
	kSystemError      = 4,     ///< 底层系统调用失败
	kIPAlreadyExists  = 5,     ///< 该 IP 已存在于目标网卡
	kIPNotFound       = 6,     ///< 目标网卡上未找到该 IP
};

/**
 * @brief 将错误码转为中文描述
 */
std::string IPErrorCodeToString(IPErrorCode code);

/**
 * @brief 单条 IP 信息
 */
struct IPInfo {
	std::string interface;     ///< 网卡名，如 "eth0"
	std::string ip;            ///< IP 地址（不含掩码），如 "192.168.1.100"
	int prefix_len = 0;        ///< CIDR 前缀长度，如 24
};

/**
 * @brief IP 操作结果
 */
struct IPResult {
	IPErrorCode code = IPErrorCode::kSuccess;
	std::string message;       ///< 中文说明信息
};

// ============================================================
//  ICMP 相关结构定义（RFC 792）
// ============================================================

/**
 * @brief 符合 RFC 792 标准的 ICMP 头部结构
 *
 * 布局（8 字节）:
 *   0       7 8     15 16                31
 *  +--------+--------+------------------+
 *  |  type  |  code  |    checksum      |
 *  +--------+--------+------------------+
 *  |       id        |    sequence      |
 *  +-----------------+------------------+
 */
struct ICMPHeader {
	uint8_t  type;        ///< 消息类型（Echo Request=8, Echo Reply=0）
	uint8_t  code;        ///< 消息代码（Echo 请求/应答均为 0）
	uint16_t checksum;    ///< 校验和
	uint16_t id;          ///< 标识符（通常使用进程 PID）
	uint16_t sequence;    ///< 序列号
};

/**
 * @brief 局域网扫描结果
 */
struct ScanResult {
	int total_scanned = 0;                ///< 总共扫描的 IP 数量
	int active_count = 0;                 ///< 活跃设备（可 Ping 通）数量
	std::vector<std::string> active_ips;  ///< 活跃设备的 IP 地址列表
	double scan_duration_ms = 0.0;        ///< 扫描耗时（毫秒）
	std::string error;                    ///< 错误信息（为空表示无错误）
};

class MyIPTools {
public:
	/**
	 * @brief 获取本机 IPv4 地址
	 * @param prefer_non_loopback 是否优先返回非回环地址
	 * @return 找到的 IPv4 地址；若未找到有效地址则回退为 127.0.0.1
	 */
	static std::string GetLocalIPv4(bool prefer_non_loopback = true);

	/**
	 * @brief 获取系统中所有可用网卡名称（排除 lo）
	 * @return 网卡名列表，如 {"eth0", "wlan0"}
	 */
	static std::vector<std::string> GetAllInterfaces();

	/**
	 * @brief 获取系统中所有 IPv4 地址与网卡的映射
	 * @param include_loopback 是否包含回环接口（默认不包含）
	 * @return IPInfo 列表
	 */
	static std::vector<IPInfo> GetAllIPs(bool include_loopback = false);

	/**
	 * @brief 向指定网卡添加 IP 地址（需 Root 权限）
	 * @param interface_name 网卡名，如 "eth0"
	 * @param cidr CIDR 格式 IP，如 "192.168.1.100/24"
	 * @return 操作结果
	 */
	static IPResult AddIP(const std::string& interface_name, const std::string& cidr);

	/**
	 * @brief 从指定网卡删除 IP 地址（需 Root 权限）
	 * @param interface_name 网卡名，如 "eth0"
	 * @param cidr CIDR 格式 IP，如 "192.168.1.100/24"
	 * @return 操作结果
	 */
	static IPResult DeleteIP(const std::string& interface_name, const std::string& cidr);

	/**
	 * @brief 校验 CIDR 格式是否合法
	 * @param cidr 待校验字符串，如 "192.168.1.100/24"
	 * @return true 格式合法
	 */
	static bool IsValidCIDR(const std::string& cidr);

	/**
	 * @brief 判断指定网卡是否存在
	 * @param interface_name 网卡名
	 * @return true 存在
	 */
	static bool InterfaceExists(const std::string& interface_name);

	// ============================================================
	//  局域网活跃设备探测
	// ============================================================

	/**
	 * @brief 初始化扫描器参数
	 * @param max_threads 最大并发线程数（默认 64）
	 * @param timeout_ms 单个 Ping 请求超时时间（毫秒，默认 800）
	 */
	static void InitScanner(int max_threads = 64, int timeout_ms = 800);

	/**
	 * @brief 扫描所有本机网段内的活跃设备
	 *
	 * 自动调用 GetAllIPs() 获取本机所有 IP / 子网，遍历各网段生成候选 IP
	 * 列表，使用原生 ICMP Raw Socket 发送 Echo Request 并等待 Reply。
	 * 线程模型为"即时线程"：为每个候选 IP 启动 std::thread，通过信号量
	 * 控制最大并发数。
	 *
	 * 需要 CAP_NET_RAW 权限或 Root。
	 *
	 * @return 扫描结果
	 */
	static ScanResult ScanActiveDevices();

	/**
	 * @brief 扫描指定网段内的活跃设备
	 *
	 * 接受 CIDR 格式（如 "192.168.2.0/24"），生成该网段的候选 IP 列表，
	 * 使用原生 ICMP Raw Socket 并发探测。
	 *
	 * 需要 CAP_NET_RAW 权限或 Root。
	 *
	 * @param cidr 目标网段，如 "192.168.2.0/24"
	 * @return 扫描结果
	 */
	static ScanResult ScanTargetSubnet(const std::string& cidr);

	/**
	 * @brief 使用原生 ICMP 探测单个 IP 是否可达
	 * @param ip 目标 IPv4 地址
	 * @param timeout_ms 超时（毫秒）
	 * @return true 可达
	 */
	static bool PingHost(const std::string& ip, int timeout_ms = 800);

	/**
	 * @brief 检测当前进程是否拥有 CAP_NET_RAW 能力或 Root 权限
	 * @return true 有权限
	 */
	static bool HasRawSocketPermission();

	/**
	 * @brief 根据 IP 和前缀长度计算网段内所有候选主机 IP
	 * @param ip 本机 IP
	 * @param prefix_len 前缀长度
	 * @return 候选 IP 列表（排除网络地址和广播地址）
	 */
	static std::vector<std::string> GenerateSubnetIPs(const std::string& ip, int prefix_len);

private:
	static bool ParseCIDR(const std::string& cidr, std::string& ip, int& prefix_len);
	static IPResult ModifyIP(const std::string& interface_name,
	                         const std::string& ip, int prefix_len, bool is_add);

	/**
	 * @brief 计算 ICMP 校验和（RFC 1071）
	 */
	static uint16_t ComputeICMPChecksum(const void* data, int len);

	/// 最大并发扫描线程数
	static std::atomic<int> s_max_threads;
	/// 单个 Ping 超时时间（毫秒）
	static std::atomic<int> s_ping_timeout_ms;
};

} // namespace my_tools
