/**
 * @file pinling_session.cpp
 * @brief 品凌吊舱会话占位实现
 *
 * 品凌吊舱的实际通信由 ViewLink SDK 完成，SDK 生命周期由 PinlingPtzCapability 管理。
 * 本 Session 仅做占位，保持 BasePod 的连接状态机正常运转。
 */

#include "pinling_session.h"
#include "MyLog.h"

namespace PodModule {

PinlingSession::PinlingSession() {
    MYLOG_INFO("[品凌会话] PinlingSession 构造");
}

PinlingSession::~PinlingSession() {
    MYLOG_INFO("[品凌会话] PinlingSession 析构");
}

PodResult<void> PinlingSession::doConnect(const std::string& host, uint16_t port) {
    MYLOG_INFO("[品凌会话] doConnect {}:{} (占位，SDK 由 PinlingPtzCapability 管理)", host, port);
    return PodResult<void>::success("品凌会话连接成功");
}

PodResult<void> PinlingSession::doDisconnect() {
    MYLOG_INFO("[品凌会话] doDisconnect (占位，SDK 由 PinlingPtzCapability 管理)");
    return PodResult<void>::success("品凌会话断开成功");
}

PodResult<void> PinlingSession::doSend(const std::vector<uint8_t>& data) {
    return PodResult<void>::success();
}

PodResult<std::vector<uint8_t>> PinlingSession::doReceive(uint32_t timeout_ms) {
    return PodResult<std::vector<uint8_t>>::success(std::vector<uint8_t>{});
}

} // namespace PodModule
