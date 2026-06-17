#include "SearchlightState.h"

#include <sstream>

#include "MyLog.h"

namespace SearchlightControl {

SearchlightState::SearchlightState()
    : presence_(DevicePresence::Unknown),
      led_(LedPowerState::Unknown),
      flash_(FlashState::Unknown),
      brightness_(-1),
      flash_hz_(-1),
      servo_x_degree_(0),
      servo_y_degree_(0),
      last_frame_counter_(0),
      last_cmd_id_(0),
      rx_frame_count_(0),
      checksum_error_count_(0),
      last_seen_ms_(0) {}

void SearchlightState::markNoDevice(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    presence_ = DevicePresence::NoDevice;
    last_error_ = reason;
}

void SearchlightState::markOnlineByFrame(const DecodedFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool first_online = presence_ != DevicePresence::Online;
    presence_ = DevicePresence::Online;
    last_seen_ms_ = nowMs();
    last_frame_counter_ = frame.frame_counter;
    last_cmd_id_ = frame.cmd_id;
    last_payload_hex_ = ProtocolCodec::toHex(frame.payload);
    last_error_.clear();
    ++rx_frame_count_;
    applyPayloadLocked(frame);

    if (first_online) {
        MYLOG_INFO("状态机：收到有效串口帧，设备状态切换为【在线】");
    }
}

void SearchlightState::markChecksumError(const std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_ = error;
    ++checksum_error_count_;
}

void SearchlightState::markCommandQueued(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_queued_command_ = command;
}

void SearchlightState::markCommandSent(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_sent_command_ = command;
}

void SearchlightState::markCommandFailed(const std::string& command, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_ = command + " 发送失败：" + reason;
}

bool SearchlightState::isOnline(int offline_timeout_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (presence_ != DevicePresence::Online) {
        return false;
    }
    if (last_seen_ms_ <= 0) {
        presence_ = DevicePresence::NoDevice;
        last_error_ = "尚未收到任何有效串口帧";
        return false;
    }
    const long long elapsed = nowMs() - last_seen_ms_;
    if (elapsed > offline_timeout_ms) {
        presence_ = DevicePresence::NoDevice;
        std::ostringstream oss;
        oss << "超过 " << offline_timeout_ms << "ms 未收到设备数据，状态切换为【无设备】";
        last_error_ = oss.str();
        MYLOG_INFO("状态机：" + last_error_);
        return false;
    }
    return true;
}

SearchlightStatusSnapshot SearchlightState::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    SearchlightStatusSnapshot item;
    item.presence = presence_;
    item.led = led_;
    item.flash = flash_;
    item.brightness = brightness_;
    item.flash_hz = flash_hz_;
    item.servo_x_degree = servo_x_degree_;
    item.servo_y_degree = servo_y_degree_;
    item.last_frame_counter = last_frame_counter_;
    item.last_cmd_id = last_cmd_id_;
    item.last_payload_hex = last_payload_hex_;
    item.last_error = last_error_;
    item.rx_frame_count = rx_frame_count_;
    item.checksum_error_count = checksum_error_count_;
    item.last_seen_ms = last_seen_ms_;
    return item;
}

std::string SearchlightState::toStatusString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "{"
        << "\"device\":\"" << presenceToText(presence_) << "\","
        << "\"led\":\"" << ledToText(led_) << "\","
        << "\"flash\":\"" << flashToText(flash_) << "\","
        << "\"brightness\":" << brightness_ << ","
        << "\"flash_hz\":" << flash_hz_ << ","
        << "\"servo_x_degree\":" << servo_x_degree_ << ","
        << "\"servo_y_degree\":" << servo_y_degree_ << ","
        << "\"last_frame_counter\":" << static_cast<int>(last_frame_counter_) << ","
        << "\"last_cmd_id\":" << static_cast<int>(last_cmd_id_) << ","
        << "\"last_payload_hex\":\"" << last_payload_hex_ << "\","
        << "\"rx_frame_count\":" << rx_frame_count_ << ","
        << "\"checksum_error_count\":" << checksum_error_count_ << ","
        << "\"last_seen_ms\":" << last_seen_ms_ << ","
        << "\"last_error\":\"" << last_error_ << "\","
        << "\"last_queued_command\":\"" << last_queued_command_ << "\","
        << "\"last_sent_command\":\"" << last_sent_command_ << "\""
        << "}";
    return oss.str();
}

std::string SearchlightState::presenceToText(DevicePresence value) {
    switch (value) {
        case DevicePresence::Unknown:
            return "未知";
        case DevicePresence::NoDevice:
            return "无设备";
        case DevicePresence::Online:
            return "在线";
    }
    return "未知";
}

std::string SearchlightState::ledToText(LedPowerState value) {
    switch (value) {
        case LedPowerState::Unknown:
            return "未知";
        case LedPowerState::Off:
            return "关闭";
        case LedPowerState::On:
            return "开启";
    }
    return "未知";
}

std::string SearchlightState::flashToText(FlashState value) {
    switch (value) {
        case FlashState::Unknown:
            return "未知";
        case FlashState::Disabled:
            return "关闭";
        case FlashState::Enabled:
            return "开启";
    }
    return "未知";
}

long long SearchlightState::nowMs() {
    const std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    return ms.count();
}

int16_t SearchlightState::readBigEndianInt16(uint8_t high, uint8_t low) {
    const uint16_t raw = static_cast<uint16_t>((static_cast<uint16_t>(high) << 8) | low);
    return static_cast<int16_t>(raw);
}

void SearchlightState::applyPayloadLocked(const DecodedFrame& frame) {
    const std::vector<uint8_t>& payload = frame.payload;
    if (payload.size() >= 4 && payload[0] == 0x2C && payload[1] == 0x74) {
        const uint8_t op = payload[2];
        const int value = static_cast<int>(payload[3]);
        switch (op) {
            case 0x00:
                flash_ = FlashState::Disabled;
                break;
            case 0x01:
                led_ = LedPowerState::On;
                flash_ = FlashState::Disabled;
                break;
            case 0x02:
                led_ = LedPowerState::Off;
                flash_ = FlashState::Disabled;
                break;
            case 0x09:
                led_ = LedPowerState::On;
                flash_ = FlashState::Enabled;
                break;
            case 0x0A:
                brightness_ = value;
                led_ = LedPowerState::On;
                break;
            case 0x0B:
                flash_hz_ = value;
                break;
            default:
                break;
        }
        return;
    }

    if (payload.size() >= 2 && payload[0] == 0x30) {
        const uint8_t op = payload[1];
        if (op == 0x04) {
            servo_x_degree_ = 0;
            servo_y_degree_ = 0;
        } else if (op == 0x0B && payload.size() >= 6) {
            const int16_t x_raw = readBigEndianInt16(payload[2], payload[3]);
            const int16_t y_raw = readBigEndianInt16(payload[4], payload[5]);
            servo_x_degree_ = static_cast<int>(x_raw / 182);
            servo_y_degree_ = static_cast<int>(-y_raw / 182);
        }
    }
}

}  // namespace SearchlightControl
