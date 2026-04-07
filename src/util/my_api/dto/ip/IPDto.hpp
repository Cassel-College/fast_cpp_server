#pragma once

/**
 * @file IPDto.hpp
 * @brief IP 管理模块的数据传输对象（DTO）定义
 *
 * 包含请求 DTO 和响应 DTO，用于 Swagger 文档自动生成。
 */

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * @brief 添加/删除 IP 请求体
 *
 * 示例 JSON:
 * {
 *   "interface": "eth0",
 *   "cidr": "192.168.1.100/24"
 * }
 */
class IPModifyRequestDto : public oatpp::DTO {
	DTO_INIT(IPModifyRequestDto, DTO)

	DTO_FIELD(String, interface_name);
	DTO_FIELD_INFO(interface_name) {
		info->description = "网卡名称，如 eth0";
		info->required = true;
	}

	DTO_FIELD(String, cidr);
	DTO_FIELD_INFO(cidr) {
		info->description = "CIDR 格式的 IP 地址，如 192.168.1.100/24";
		info->required = true;
	}
};

/**
 * @brief 局域网扫描配置请求体（可选）
 *
 * 示例 JSON:
 * {
 *   "max_threads": 64,
 *   "timeout_ms": 800
 * }
 */
class ScanConfigRequestDto : public oatpp::DTO {
	DTO_INIT(ScanConfigRequestDto, DTO)

	DTO_FIELD(Int32, max_threads);
	DTO_FIELD_INFO(max_threads) {
		info->description = "最大并发扫描线程数（1-1024，默认 64）";
		info->required = false;
	}

	DTO_FIELD(Int32, timeout_ms);
	DTO_FIELD_INFO(timeout_ms) {
		info->description = "单个 Ping 请求超时时间（100-5000 毫秒，默认 800）";
		info->required = false;
	}
};

/**
 * @brief 指定网段扫描请求体
 *
 * 示例 JSON:
 * {
 *   "cidr": "192.168.2.0/24"
 * }
 */
class ScanTargetRequestDto : public oatpp::DTO {
	DTO_INIT(ScanTargetRequestDto, DTO)

	DTO_FIELD(String, cidr);
	DTO_FIELD_INFO(cidr) {
		info->description = "目标网段，CIDR 格式，如 192.168.2.0/24 或 192.168.2.119/24";
		info->required = true;
	}
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto
