#pragma once

/**
 * @file pinling_session.h
 * @brief 品凌吊舱会话实现
 *
 * 品凌吊舱通过 ViewLink SDK 直接与设备通信，SDK 的生命周期由 PinlingPtzCapability 管理。
 * 本 Session 仅作为占位实现，满足 BasePod 对 Session 的结构约束。
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
