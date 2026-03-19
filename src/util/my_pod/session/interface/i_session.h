#pragma once

/**
 * @file i_session.h
 * @brief 会话接口定义
 * 
 * 定义吊舱底层通信会话的统一抽象接口。
 * 所有厂商会话实现必须继承此接口。
 */

#include "../../common/pod_result.h"
#include <string>
#include <vector>
#include <cstdint>

namespace PodModule {

/**
 * @brief 会话接口
 * 
 * 负责与吊舱设备的底层通信，包括连接管理和数据收发。
 */
class ISession {
public:
    virtual ~ISession() = default;

    /** @brief 建立与设备的连接 */
    virtual PodResult<void> connect(const std::string& host, uint16_t port) = 0;

    /** @brief 断开与设备的连接 */
    virtual PodResult<void> disconnect() = 0;

    /** @brief 判断当前是否已连接 */
    virtual bool isConnected() const = 0;

    /**
     * @brief 发送请求数据
     * @param data 要发送的原始数据
     * @return 操作结果
     */
    virtual PodResult<void> sendRequest(const std::vector<uint8_t>& data) = 0;

    /**
     * @brief 接收响应数据
     * @param timeout_ms 超时时间（毫秒），0表示不超时
     * @return 接收到的数据
     */
    virtual PodResult<std::vector<uint8_t>> receiveResponse(uint32_t timeout_ms = 3000) = 0;

    /**
     * @brief 发送请求并等待响应（请求-应答模式）
     * @param request 请求数据
     * @param timeout_ms 超时时间（毫秒）
     * @return 响应数据
     */
    virtual PodResult<std::vector<uint8_t>> request(const std::vector<uint8_t>& request,
                                                     uint32_t timeout_ms = 3000) = 0;
};

} // namespace PodModule
