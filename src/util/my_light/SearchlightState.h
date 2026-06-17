#ifndef SEARCHLIGHT_STATE_H
#define SEARCHLIGHT_STATE_H

#include "SearchlightProtocol.h"

#include <stdint.h>

#include <chrono>
#include <mutex>
#include <string>

namespace SearchlightControl {

enum class DevicePresence {
    Unknown,
    NoDevice,
    Online
};

enum class LedPowerState {
    Unknown,
    Off,
    On
};

enum class FlashState {
    Unknown,
    Disabled,
    Enabled
};

struct SearchlightStatusSnapshot {
    DevicePresence presence;
    LedPowerState led;
    FlashState flash;
    int brightness;
    int flash_hz;
    int servo_x_degree;
    int servo_y_degree;
    uint8_t last_frame_counter;
    uint8_t last_cmd_id;
    std::string last_payload_hex;
    std::string last_error;
    uint64_t rx_frame_count;
    uint64_t checksum_error_count;
    long long last_seen_ms;
};

// 状态类：维护设备存在性、LED 状态、闪烁状态、亮度和伺服角度。
class SearchlightState {
public:
    SearchlightState();

    void markNoDevice(const std::string& reason);
    void markOnlineByFrame(const DecodedFrame& frame);
    void markChecksumError(const std::string& error);
    void markCommandQueued(const std::string& command);
    void markCommandSent(const std::string& command);
    void markCommandFailed(const std::string& command, const std::string& reason);
    bool isOnline(int offline_timeout_ms);
    SearchlightStatusSnapshot snapshot() const;
    std::string toStatusString() const;

private:
    static std::string presenceToText(DevicePresence value);
    static std::string ledToText(LedPowerState value);
    static std::string flashToText(FlashState value);
    static long long nowMs();
    static int16_t readBigEndianInt16(uint8_t high, uint8_t low);

    void applyPayloadLocked(const DecodedFrame& frame);

    mutable std::mutex mutex_;
    DevicePresence presence_;
    LedPowerState led_;
    FlashState flash_;
    int brightness_;
    int flash_hz_;
    int servo_x_degree_;
    int servo_y_degree_;
    uint8_t last_frame_counter_;
    uint8_t last_cmd_id_;
    std::string last_payload_hex_;
    std::string last_error_;
    uint64_t rx_frame_count_;
    uint64_t checksum_error_count_;
    long long last_seen_ms_;
    std::string last_queued_command_;
    std::string last_sent_command_;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_STATE_H
