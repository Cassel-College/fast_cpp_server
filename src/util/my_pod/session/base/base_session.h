#pragma once

/**
 * @file base_session.h
 * @brief 基础会话实现
 * 
 * 提供 ISession 的基础实现，包含通用的连接状态管理和日志。
 * 厂商会话继承此类并实现具体通信逻辑。
 */

#include "../interface/i_session.h"
#include <string>
#include <mutex>

namespace PodModule {

/**
 * @brief 基础会话类
 * 
 * 提供连接状态管理、基础日志等通用功能。
 */
class BaseSession : public ISession {
public:
    BaseSession();
    ~BaseSession() override;

    // --- ISession 接口实现 ---
    PodResult<void> connect(const std::string& host, uint16_t port) override;
    PodResult<void> disconnect() override;
    bool isConnected() const override;
    PodResult<void> sendRequest(const std::vector<uint8_t>& data) override;
    PodResult<std::vector<uint8_t>> receiveResponse(uint32_t timeout_ms = 3000) override;
    PodResult<std::vector<uint8_t>> request(const std::vector<uint8_t>& request,
                                             uint32_t timeout_ms = 3000) override;

protected:
    /** @brief 子类实现的实际连接逻辑 */
    virtual PodResult<void> doConnect(const std::string& host, uint16_t port);

    /** @brief 子类实现的实际断开逻辑 */
    virtual PodResult<void> doDisconnect();

    /** @brief 子类实现的实际发送逻辑 */
    virtual PodResult<void> doSend(const std::vector<uint8_t>& data);

    /** @brief 子类实现的实际接收逻辑 */
    virtual PodResult<std::vector<uint8_t>> doReceive(uint32_t timeout_ms);

    std::string host_;          // 目标主机地址
    uint16_t    port_ = 0;      // 目标端口
    bool        connected_ = false;  // 连接状态
    mutable std::mutex mutex_;  // 线程安全互斥锁
};

} // namespace PodModule
