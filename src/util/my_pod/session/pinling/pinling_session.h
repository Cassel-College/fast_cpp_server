#pragma once

/**
 * @file pinling_session.h
 * @brief 品凌吊舱会话实现
 * 
 * 封装与品凌吊舱设备的底层通信协议。
 * 当前为占位实现，后续对接品凌设备协议。
 */

#include "../base/base_session.h"

namespace PodModule {

class PinlingSession : public BaseSession {
public:
    PinlingSession();
    ~PinlingSession() override;

protected:
    PodResult<void> doConnect(const std::string& host, uint16_t port) override;
    PodResult<void> doDisconnect() override;
    PodResult<void> doSend(const std::vector<uint8_t>& data) override;
    PodResult<std::vector<uint8_t>> doReceive(uint32_t timeout_ms) override;
};

} // namespace PodModule
