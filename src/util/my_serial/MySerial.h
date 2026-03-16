#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <serial/serial.h>

namespace my_serial {

struct SerialInitOptions {
    std::string port;
    uint32_t baudrate{9600};
    uint32_t timeout_ms{1000};
    uint32_t inter_byte_timeout_ms{0};
    uint32_t read_timeout_constant_ms{1000};
    uint32_t read_timeout_multiplier_ms{0};
    uint32_t write_timeout_constant_ms{1000};
    uint32_t write_timeout_multiplier_ms{0};
    std::string bytesize{"eightbits"};
    std::string parity{"none"};
    std::string stopbits{"one"};
    std::string flowcontrol{"none"};
    bool auto_open{true};
};

struct SerialPortSnapshot {
    bool initialized{false};
    bool open{false};
    std::string port;
    uint32_t baudrate{0};
    uint32_t timeout_ms{0};
    size_t available_bytes{0};
    std::string bytesize;
    std::string parity;
    std::string stopbits;
    std::string flowcontrol;
    std::string last_error;
};

struct SerialSelfCheckResult {
    bool success{false};
    std::string summary;
    nlohmann::json detail;
};

class MySerial {
public:
    MySerial();
    ~MySerial();

    bool Init(const nlohmann::json& cfg, std::string* err = nullptr);
    bool Open(std::string* err = nullptr);
    void Close();
    bool IsOpen() const;

    size_t Write(const std::string& data, std::string* err = nullptr);
    size_t Write(const std::vector<uint8_t>& data, std::string* err = nullptr);
    std::string Read(size_t size, std::string* err = nullptr);
    std::vector<uint8_t> ReadBytes(size_t size, std::string* err = nullptr);
    std::string ReadLine(size_t max_size = 65536, const std::string& eol = "\n", std::string* err = nullptr);
    size_t Available(std::string* err = nullptr) const;
    std::vector<nlohmann::json> ListAvailablePorts() const;

    SerialPortSnapshot GetSnapshot() const;
    nlohmann::json GetSnapshotJson() const;
    SerialSelfCheckResult SelfCheck() const;
    SerialSelfCheckResult SelfTestLoopback(const std::string& payload, size_t read_size = 0);

private:
    static SerialInitOptions ParseOptions(const nlohmann::json& cfg);
    static serial::bytesize_t ParseBytesize(const std::string& value);
    static serial::parity_t ParseParity(const std::string& value);
    static serial::stopbits_t ParseStopbits(const std::string& value);
    static serial::flowcontrol_t ParseFlowcontrol(const std::string& value);

    static std::string ToString(serial::bytesize_t value);
    static std::string ToString(serial::parity_t value);
    static std::string ToString(serial::stopbits_t value);
    static std::string ToString(serial::flowcontrol_t value);

private:
    mutable std::mutex mutex_;
    bool initialized_{false};
    mutable std::string last_error_;
    SerialInitOptions options_;
    std::unique_ptr<serial::Serial> serial_;
};

} // namespace my_serial