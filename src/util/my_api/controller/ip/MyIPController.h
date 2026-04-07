#pragma once

/**
 * @file MyIPController.h
 * @brief IP 管理模块 REST API 控制器
 *
 * 对外暴露以下接口：
 * - GET  /v1/ip/interfaces   : 获取所有可用网卡列表
 * - GET  /v1/ip/addresses    : 获取所有 IP 地址信息
 * - POST /v1/ip/add          : 向指定网卡添加 IP（需 Root）
 * - POST /v1/ip/delete       : 从指定网卡删除 IP（需 Root）
 * - POST /v1/ip/scan         : 扫描局域网活跃设备（需 CAP_NET_RAW / Root）
 * - POST /v1/ip/scan_target_ip : 扫描指定网段活跃设备（需 CAP_NET_RAW / Root）
 * - POST /v1/ip/scan/config  : 配置扫描参数
 */

#include "BaseApiController.hpp"
#include "dto/ip/IPDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

namespace my_api::ip_api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class MyIPController : public base::BaseApiController {
public:
	using MyAPIResponsePtr = my_api::base::MyAPIResponsePtr;
	explicit MyIPController(const std::shared_ptr<ObjectMapper>& objectMapper);

	static std::shared_ptr<MyIPController> createShared(
		const std::shared_ptr<ObjectMapper>& objectMapper);

	// ==================== 查询类接口 ====================

	ENDPOINT_INFO(getAllInterfaces) {
		info->summary = "获取所有可用网卡列表";
		info->description = "返回系统中所有处于 UP 状态的非回环网卡名称列表。";
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
	}
	ENDPOINT("GET", "/v1/ip/interfaces", getAllInterfaces);

	ENDPOINT_INFO(getAllAddresses) {
		info->summary = "获取所有 IP 地址信息";
		info->description = "返回系统中所有 IPv4 地址及其对应网卡和前缀长度。\n"
		                    "响应 data 字段为 IPInfo 数组：\n"
		                    "[{\"interface\":\"eth0\", \"ip\":\"192.168.1.10\", \"prefix_len\":24}, ...]";
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
	}
	ENDPOINT("GET", "/v1/ip/addresses", getAllAddresses);

	// ==================== 操作类接口 ====================

	ENDPOINT_INFO(addIP) {
		info->summary = "向指定网卡添加 IP 地址";
		info->description = "使用 rtnetlink 向指定网卡添加一个 CIDR 格式的 IPv4 地址。\n"
		                    "需要 Root 权限。\n"
		                    "请求体示例：{\"interface_name\":\"eth0\", \"cidr\":\"192.168.1.100/24\"}";
		info->addConsumes<oatpp::Object<my_api::dto::IPModifyRequestDto>>("application/json");
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
	}
	ENDPOINT("POST", "/v1/ip/add", addIP,
	         BODY_DTO(oatpp::Object<my_api::dto::IPModifyRequestDto>, requestDto));

	ENDPOINT_INFO(deleteIP) {
		info->summary = "从指定网卡删除 IP 地址";
		info->description = "使用 rtnetlink 从指定网卡删除一个 CIDR 格式的 IPv4 地址。\n"
		                    "需要 Root 权限。\n"
		                    "请求体示例：{\"interface_name\":\"eth0\", \"cidr\":\"192.168.1.100/24\"}";
		info->addConsumes<oatpp::Object<my_api::dto::IPModifyRequestDto>>("application/json");
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_404, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
	}
	ENDPOINT("POST", "/v1/ip/delete", deleteIP,
	         BODY_DTO(oatpp::Object<my_api::dto::IPModifyRequestDto>, requestDto));

	// ==================== 局域网扫描接口 ====================

	ENDPOINT_INFO(scanDevices) {
		info->summary = "扫描局域网活跃设备";
		info->description = "使用原生 ICMP Echo Request 扫描所有本机网段内的活跃设备。\n"
		                    "需要 CAP_NET_RAW 权限或 Root 权限。\n"
		                    "返回字段：total_scanned（扫描总数）、active_count（活跃数）、\n"
		                    "active_ips（活跃 IP 列表）、scan_duration_ms（耗时毫秒）。\n"
		                    "注意：扫描大网段时可能耗时较长，建议先通过 /v1/ip/scan/config 配置参数。";
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_500, "application/json");
	}
	ENDPOINT("GET", "/v1/ip/scan", scanDevices);

	ENDPOINT_INFO(scanTargetIP) {
		info->summary = "扫描指定网段的活跃设备";
		info->description = "对指定 CIDR 网段执行 ICMP 扫描，返回在线设备 IP 列表。\n"
		                    "需要 CAP_NET_RAW 权限或 Root 权限。\n"
		                    "请求体示例：{\"cidr\":\"192.168.2.119/24\"}";
		info->addConsumes<oatpp::Object<my_api::dto::ScanTargetRequestDto>>("application/json");
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_403, "application/json");
	}
	ENDPOINT("POST", "/v1/ip/scan_target_ip", scanTargetIP,
	         BODY_DTO(oatpp::Object<my_api::dto::ScanTargetRequestDto>, requestDto));

	ENDPOINT_INFO(configureScan) {
		info->summary = "配置局域网扫描参数";
		info->description = "设置扫描器的最大并发线程数和单次 Ping 超时时间。\n"
		                    "请求体示例：{\"max_threads\":64, \"timeout_ms\":800}";
		info->addConsumes<oatpp::Object<my_api::dto::ScanConfigRequestDto>>("application/json");
		info->addResponse<oatpp::String>(Status::CODE_200, "application/json");
		info->addResponse<oatpp::String>(Status::CODE_400, "application/json");
	}
	ENDPOINT("POST", "/v1/ip/scan/config", configureScan,
	         BODY_DTO(oatpp::Object<my_api::dto::ScanConfigRequestDto>, configDto));
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace my_api::ip_api
