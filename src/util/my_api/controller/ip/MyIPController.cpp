#include "MyIPController.h"

#include "MyIPTools.h"
#include "MyLog.h"
#include <nlohmann/json.hpp>

namespace my_api::ip_api {

using namespace my_api::base;

MyIPController::MyIPController(const std::shared_ptr<ObjectMapper>& objectMapper)
	: BaseApiController(objectMapper) {}

std::shared_ptr<MyIPController> MyIPController::createShared(
	const std::shared_ptr<ObjectMapper>& objectMapper) {
	return std::make_shared<MyIPController>(objectMapper);
}

// ============================================================
//  辅助：根据 IPErrorCode 映射 HTTP 状态码
// ============================================================

namespace {

oatpp::web::protocol::http::Status MapIPErrorToHTTPStatus(my_tools::IPErrorCode code) {
	using Status = oatpp::web::protocol::http::Status;
	switch (code) {
		case my_tools::IPErrorCode::kSuccess:           return Status::CODE_200;
		case my_tools::IPErrorCode::kInvalidCIDR:       return Status::CODE_400;
		case my_tools::IPErrorCode::kInterfaceNotFound:  return Status::CODE_400;
		case my_tools::IPErrorCode::kPermissionDenied:  return Status::CODE_403;
		case my_tools::IPErrorCode::kIPAlreadyExists:   return Status::CODE_409;
		case my_tools::IPErrorCode::kIPNotFound:        return Status::CODE_404;
		case my_tools::IPErrorCode::kSystemError:       return Status::CODE_500;
		default:                                        return Status::CODE_500;
	}
}

} // anonymous namespace

// ============================================================
//  GET /v1/ip/interfaces
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::getAllInterfaces() {
	MYLOG_INFO("[IP API] 收到获取网卡列表请求");

	auto interfaces = my_tools::MyIPTools::GetAllInterfaces();

	nlohmann::json data = nlohmann::json::array();
	for (const auto& iface : interfaces) {
		data.push_back(iface);
	}

	return jsonOk(data, "获取网卡列表成功");
}

// ============================================================
//  GET /v1/ip/addresses
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::getAllAddresses() {
	MYLOG_INFO("[IP API] 收到获取所有 IP 地址请求");

	auto ips = my_tools::MyIPTools::GetAllIPs(false);

	nlohmann::json data = nlohmann::json::array();
	for (const auto& info : ips) {
		nlohmann::json item;
		item["interface"]  = info.interface;
		item["ip"]         = info.ip;
		item["prefix_len"] = info.prefix_len;
		data.push_back(item);
	}

	return jsonOk(data, "获取 IP 地址列表成功");
}

// ============================================================
//  POST /v1/ip/add
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::addIP(
	const oatpp::Object<my_api::dto::IPModifyRequestDto>& requestDto) {

	// 参数校验
	if (!requestDto->interface_name || !requestDto->cidr) {
		MYLOG_WARN("[IP API] 添加 IP 请求缺少必要参数");
		return jsonError(400, "缺少必要参数: interface_name 和 cidr 不能为空");
	}

	std::string iface = requestDto->interface_name->c_str();
	std::string cidr  = requestDto->cidr->c_str();

	MYLOG_INFO("[IP API] 收到添加 IP 请求: 网卡={}, CIDR={}", iface, cidr);

	auto result = my_tools::MyIPTools::AddIP(iface, cidr);
	auto status = MapIPErrorToHTTPStatus(result.code);

	if (result.code == my_tools::IPErrorCode::kSuccess) {
		return jsonOk(nlohmann::json::object(), result.message);
	}

	return jsonError(static_cast<int>(result.code), result.message);
}

// ============================================================
//  POST /v1/ip/delete
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::deleteIP(
	const oatpp::Object<my_api::dto::IPModifyRequestDto>& requestDto) {

	// 参数校验
	if (!requestDto->interface_name || !requestDto->cidr) {
		MYLOG_WARN("[IP API] 删除 IP 请求缺少必要参数");
		return jsonError(400, "缺少必要参数: interface_name 和 cidr 不能为空");
	}

	std::string iface = requestDto->interface_name->c_str();
	std::string cidr  = requestDto->cidr->c_str();

	MYLOG_INFO("[IP API] 收到删除 IP 请求: 网卡={}, CIDR={}", iface, cidr);

	auto result = my_tools::MyIPTools::DeleteIP(iface, cidr);
	auto status = MapIPErrorToHTTPStatus(result.code);

	if (result.code == my_tools::IPErrorCode::kSuccess) {
		return jsonOk(nlohmann::json::object(), result.message);
	}

	return jsonError(static_cast<int>(result.code), result.message);
}

// ============================================================
//  GET /v1/ip/scan
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::scanDevices() {
	MYLOG_INFO("[IP API] 收到局域网扫描请求");

	auto scan_result = my_tools::MyIPTools::ScanActiveDevices();

	if (!scan_result.error.empty()) {
		MYLOG_WARN("[IP API] 扫描失败: {}", scan_result.error);
		return jsonError(403, scan_result.error);
	}

	nlohmann::json data;
	data["total_scanned"]    = scan_result.total_scanned;
	data["active_count"]     = scan_result.active_count;
	data["scan_duration_ms"] = scan_result.scan_duration_ms;

	nlohmann::json ips = nlohmann::json::array();
	for (const auto& ip : scan_result.active_ips) {
		ips.push_back(ip);
	}
	data["active_ips"] = ips;

	return jsonOk(data, "局域网扫描完成");
}

// ============================================================
//  POST /v1/ip/scan_target_ip
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::scanTargetIP(
	const oatpp::Object<my_api::dto::ScanTargetRequestDto>& requestDto) {

	if (!requestDto->cidr) {
		MYLOG_WARN("[IP API] 指定网段扫描请求缺少 cidr 参数");
		return jsonError(400, "缺少必要参数: cidr 不能为空，示例: 192.168.2.0/24");
	}

	std::string cidr = requestDto->cidr->c_str();
	MYLOG_INFO("[IP API] 收到指定网段扫描请求: {}", cidr);

	if (!my_tools::MyIPTools::IsValidCIDR(cidr)) {
		return jsonError(400, "CIDR 格式非法: " + cidr + "，示例: 192.168.2.0/24");
	}

	auto scan_result = my_tools::MyIPTools::ScanTargetSubnet(cidr);

	if (!scan_result.error.empty()) {
		MYLOG_WARN("[IP API] 指定网段扫描失败: {}", scan_result.error);
		return jsonError(403, scan_result.error);
	}

	nlohmann::json data;
	data["cidr"]             = cidr;
	data["total_scanned"]    = scan_result.total_scanned;
	data["active_count"]     = scan_result.active_count;
	data["scan_duration_ms"] = scan_result.scan_duration_ms;

	nlohmann::json ips = nlohmann::json::array();
	for (const auto& ip : scan_result.active_ips) {
		ips.push_back(ip);
	}
	data["active_ips"] = ips;

	return jsonOk(data, "指定网段扫描完成");
}

// ============================================================
//  POST /v1/ip/scan/config
// ============================================================

MyIPController::MyAPIResponsePtr MyIPController::configureScan(
	const oatpp::Object<my_api::dto::ScanConfigRequestDto>& configDto) {

	int max_threads = 64;
	int timeout_ms  = 800;

	if (configDto->max_threads) {
		max_threads = *configDto->max_threads;
	}
	if (configDto->timeout_ms) {
		timeout_ms = *configDto->timeout_ms;
	}

	if (max_threads < 1 || max_threads > 1024) {
		return jsonError(400, "max_threads 必须在 1-1024 范围内");
	}
	if (timeout_ms < 100 || timeout_ms > 5000) {
		return jsonError(400, "timeout_ms 必须在 100-5000 范围内");
	}

	MYLOG_INFO("[IP API] 配置扫描参数: max_threads={}, timeout_ms={}", max_threads, timeout_ms);
	my_tools::MyIPTools::InitScanner(max_threads, timeout_ms);

	nlohmann::json data;
	data["max_threads"] = max_threads;
	data["timeout_ms"]  = timeout_ms;

	return jsonOk(data, "扫描参数配置成功");
}

} // namespace my_api::ip_api
