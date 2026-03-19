#pragma once

/**
 * @file dji_session.h
 * @brief 大疆吊舱会话实现
 * 
 * 封装与大疆吊舱设备的底层通信协议。
 * 当前为占位实现，后续对接实际大疆SDK/协议。
 */

#include "../base/base_session.h"

namespace PodModule {

class DjiSession : public BaseSession {
public:
    DjiSession();
    ~DjiSession() override;

protected:
    PodResult<void> doConnect(const std::string& host, uint16_t port) override;
    PodResult<void> doDisconnect() override;
    PodResult<void> doSend(const std::vector<uint8_t>& data) override;
    PodResult<std::vector<uint8_t>> doReceive(uint32_t timeout_ms) override;
};

} // namespace PodModule
