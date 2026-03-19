/**
 * @file base_session.cpp
 * @brief 基础会话实现
 */

#include "base_session.h"
#include "MyLog.h"

namespace PodModule {

BaseSession::BaseSession() {
    MYLOG_DEBUG("[吊舱会话] BaseSession 构造");
}

BaseSession::~BaseSession() {
    MYLOG_DEBUG("[吊舱会话] BaseSession 析构");
    if (connected_) {
        disconnect();
    }
}

PodResult<void> BaseSession::connect(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_) {
        MYLOG_WARN("[吊舱会话] 已处于连接状态，目标: {}:{}", host, port);
        return PodResult<void>::fail(PodErrorCode::ALREADY_CONNECTED, "会话已连接");
    }

    MYLOG_INFO("[吊舱会话] 正在连接 {}:{}", host, port);
    host_ = host;
    port_ = port;

    auto result = doConnect(host, port);
    if (result.isSuccess()) {
        connected_ = true;
        MYLOG_INFO("[吊舱会话] 连接成功 {}:{}", host, port);
    } else {
        MYLOG_ERROR("[吊舱会话] 连接失败 {}:{} - {}", host, port, result.message);
    }
    return result;
}

PodResult<void> BaseSession::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_) {
        MYLOG_WARN("[吊舱会话] 当前未连接，无需断开");
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED, "会话未连接");
    }

    MYLOG_INFO("[吊舱会话] 正在断开连接 {}:{}", host_, port_);
    auto result = doDisconnect();
    connected_ = false;
    MYLOG_INFO("[吊舱会话] 已断开连接");
    return result;
}

bool BaseSession::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

PodResult<void> BaseSession::sendRequest(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_) {
        MYLOG_ERROR("[吊舱会话] 发送失败：未连接");
        return PodResult<void>::fail(PodErrorCode::NOT_CONNECTED);
    }
    return doSend(data);
}

PodResult<std::vector<uint8_t>> BaseSession::receiveResponse(uint32_t timeout_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_) {
        MYLOG_ERROR("[吊舱会话] 接收失败：未连接");
        return PodResult<std::vector<uint8_t>>::fail(PodErrorCode::NOT_CONNECTED);
    }
    return doReceive(timeout_ms);
}

PodResult<std::vector<uint8_t>> BaseSession::request(const std::vector<uint8_t>& req,
                                                      uint32_t timeout_ms) {
    // 请求-应答模式：先发送再接收
    auto sendResult = sendRequest(req);
    if (!sendResult.isSuccess()) {
        return PodResult<std::vector<uint8_t>>::fail(sendResult.code, sendResult.message);
    }
    return receiveResponse(timeout_ms);
}

// --- 默认的 do* 占位实现（子类可覆盖）---

PodResult<void> BaseSession::doConnect(const std::string& host, uint16_t port) {
    MYLOG_DEBUG("[吊舱会话] BaseSession::doConnect 占位实现 {}:{}", host, port);
    return PodResult<void>::success("基础连接成功（占位）");
}

PodResult<void> BaseSession::doDisconnect() {
    MYLOG_DEBUG("[吊舱会话] BaseSession::doDisconnect 占位实现");
    return PodResult<void>::success("基础断开成功（占位）");
}

PodResult<void> BaseSession::doSend(const std::vector<uint8_t>& data) {
    MYLOG_DEBUG("[吊舱会话] BaseSession::doSend 占位实现，数据长度: {}", data.size());
    return PodResult<void>::success("发送成功（占位）");
}

PodResult<std::vector<uint8_t>> BaseSession::doReceive(uint32_t timeout_ms) {
    MYLOG_DEBUG("[吊舱会话] BaseSession::doReceive 占位实现，超时: {}ms", timeout_ms);
    std::vector<uint8_t> dummy = {0x00};
    return PodResult<std::vector<uint8_t>>::success(dummy, "接收成功（占位）");
}

} // namespace PodModule
