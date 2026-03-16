#include "MySerial.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "MyLog.h"

namespace my_serial {

namespace {

std::string NormalizeToken(const std::string& value) {
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return normalized;
}

serial::Timeout BuildTimeout(const SerialInitOptions& options) {
    if (options.inter_byte_timeout_ms == 0 &&
        options.read_timeout_constant_ms == options.timeout_ms &&
        options.read_timeout_multiplier_ms == 0 &&
        options.write_timeout_constant_ms == options.timeout_ms &&
        options.write_timeout_multiplier_ms == 0) {
        return serial::Timeout::simpleTimeout(options.timeout_ms);
    }

    return serial::Timeout(
        options.inter_byte_timeout_ms,
        options.read_timeout_constant_ms,
        options.read_timeout_multiplier_ms,
        options.write_timeout_constant_ms,
        options.write_timeout_multiplier_ms);
}

template <typename Fn>
bool ExecuteWithError(std::string* err, std::string& last_error, Fn&& fn) {
    try {
        fn();
        last_error.clear();
        if (err != nullptr) {
            err->clear();
        }
        return true;
    } catch (const std::exception& ex) {
        last_error = ex.what();
        if (err != nullptr) {
            *err = last_error;
        }
        return false;
    }
}

} // namespace

MySerial::MySerial() = default;

MySerial::~MySerial() {
    Close();
}

bool MySerial::Init(const nlohmann::json& cfg, std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);

    return ExecuteWithError(err, last_error_, [this, &cfg]() {
        if (serial_ && serial_->isOpen()) {
            serial_->close();
        }

        initialized_ = false;
        options_ = ParseOptions(cfg);

        serial_ = std::make_unique<serial::Serial>();
        serial_->setPort(options_.port);
        serial_->setBaudrate(options_.baudrate);

        serial::Timeout timeout = BuildTimeout(options_);
        serial_->setTimeout(timeout);
        serial_->setBytesize(ParseBytesize(options_.bytesize));
        serial_->setParity(ParseParity(options_.parity));
        serial_->setStopbits(ParseStopbits(options_.stopbits));
        serial_->setFlowcontrol(ParseFlowcontrol(options_.flowcontrol));

        MYLOG_INFO("[MySerial] Init success: port={}, baudrate={}, auto_open={}",
                   options_.port, options_.baudrate, options_.auto_open ? "true" : "false");

        if (options_.auto_open) {
            serial_->open();
            MYLOG_INFO("[MySerial] Port opened during Init: {}", options_.port);
        }

        initialized_ = true;
    });
}

bool MySerial::Open(std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);

    return ExecuteWithError(err, last_error_, [this]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            serial_->open();
            MYLOG_INFO("[MySerial] Port opened: {}", options_.port);
        }
    });
}

void MySerial::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serial_ && serial_->isOpen()) {
        serial_->close();
        MYLOG_INFO("[MySerial] Port closed: {}", options_.port);
    }
}

bool MySerial::IsOpen() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return serial_ && serial_->isOpen();
}

size_t MySerial::Write(const std::string& data, std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t written = 0;
    ExecuteWithError(err, last_error_, [this, &data, &written]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            throw std::runtime_error("Serial port is not open");
        }
        written = serial_->write(data);
    });
    return written;
}

size_t MySerial::Write(const std::vector<uint8_t>& data, std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t written = 0;
    ExecuteWithError(err, last_error_, [this, &data, &written]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            throw std::runtime_error("Serial port is not open");
        }
        written = serial_->write(data);
    });
    return written;
}

std::string MySerial::Read(size_t size, std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    ExecuteWithError(err, last_error_, [this, size, &result]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            throw std::runtime_error("Serial port is not open");
        }
        result = serial_->read(size);
    });
    return result;
}

std::vector<uint8_t> MySerial::ReadBytes(size_t size, std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> result;
    ExecuteWithError(err, last_error_, [this, size, &result]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            throw std::runtime_error("Serial port is not open");
        }
        result.resize(size);
        const size_t read_size = serial_->read(result, size);
        result.resize(read_size);
    });
    return result;
}

std::string MySerial::ReadLine(size_t max_size, const std::string& eol, std::string* err) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    ExecuteWithError(err, last_error_, [this, max_size, &eol, &result]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            throw std::runtime_error("Serial port is not open");
        }
        result = serial_->readline(max_size, eol);
    });
    return result;
}

size_t MySerial::Available(std::string* err) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t available = 0;
    ExecuteWithError(err, last_error_, [this, &available]() {
        if (!initialized_ || !serial_) {
            throw std::runtime_error("MySerial is not initialized");
        }
        if (!serial_->isOpen()) {
            throw std::runtime_error("Serial port is not open");
        }
        available = serial_->available();
    });
    return available;
}

std::vector<nlohmann::json> MySerial::ListAvailablePorts() const {
    std::vector<nlohmann::json> ports_json;
    for (const auto& port_info : serial::list_ports()) {
        ports_json.push_back({
            {"port", port_info.port},
            {"description", port_info.description},
            {"hardware_id", port_info.hardware_id}
        });
    }
    return ports_json;
}

SerialPortSnapshot MySerial::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    SerialPortSnapshot snapshot;
    snapshot.initialized = initialized_;
    snapshot.last_error = last_error_;

    if (!initialized_ || !serial_) {
        return snapshot;
    }

    snapshot.open = serial_->isOpen();
    snapshot.port = options_.port;
    snapshot.baudrate = options_.baudrate;
    snapshot.timeout_ms = options_.timeout_ms;
    snapshot.bytesize = options_.bytesize;
    snapshot.parity = options_.parity;
    snapshot.stopbits = options_.stopbits;
    snapshot.flowcontrol = options_.flowcontrol;

    if (snapshot.open) {
        try {
            snapshot.available_bytes = serial_->available();
        } catch (const std::exception& ex) {
            snapshot.last_error = ex.what();
        }
    }

    return snapshot;
}

nlohmann::json MySerial::GetSnapshotJson() const {
    const SerialPortSnapshot snapshot = GetSnapshot();
    return {
        {"initialized", snapshot.initialized},
        {"open", snapshot.open},
        {"port", snapshot.port},
        {"baudrate", snapshot.baudrate},
        {"timeout_ms", snapshot.timeout_ms},
        {"available_bytes", snapshot.available_bytes},
        {"bytesize", snapshot.bytesize},
        {"parity", snapshot.parity},
        {"stopbits", snapshot.stopbits},
        {"flowcontrol", snapshot.flowcontrol},
        {"last_error", snapshot.last_error}
    };
}

SerialSelfCheckResult MySerial::SelfCheck() const {
    SerialSelfCheckResult result;
    result.detail["snapshot"] = GetSnapshotJson();
    result.detail["available_ports"] = ListAvailablePorts();

    const SerialPortSnapshot snapshot = GetSnapshot();
    if (!snapshot.initialized) {
        result.summary = "serial not initialized";
        result.detail["checks"] = {
            {"initialized", false},
            {"open", false}
        };
        return result;
    }

    bool port_found = false;
    for (const auto& port_item : result.detail["available_ports"]) {
        if (port_item.value("port", std::string()) == snapshot.port) {
            port_found = true;
            break;
        }
    }

    result.success = snapshot.open;
    result.summary = snapshot.open ? "serial self check passed" : "serial self check failed";
    result.detail["checks"] = {
        {"initialized", snapshot.initialized},
        {"open", snapshot.open},
        {"configured_port_found", port_found},
        {"available_bytes_non_negative", true}
    };
    return result;
}

SerialSelfCheckResult MySerial::SelfTestLoopback(const std::string& payload, size_t read_size) {
    SerialSelfCheckResult result;
    std::string err;

    const size_t expected_size = read_size == 0 ? payload.size() : read_size;
    const size_t written = Write(payload, &err);
    if (!err.empty()) {
        result.summary = "write failed: " + err;
        result.detail["payload"] = payload;
        return result;
    }

    const std::string received = Read(expected_size, &err);
    if (!err.empty()) {
        result.summary = "read failed: " + err;
        result.detail["payload"] = payload;
        result.detail["written_size"] = written;
        return result;
    }

    result.success = (received == payload);
    result.summary = result.success ? "serial loopback self test passed" : "serial loopback self test mismatch";
    result.detail = {
        {"payload", payload},
        {"written_size", written},
        {"expected_size", expected_size},
        {"received", received},
        {"snapshot", GetSnapshotJson()}
    };
    return result;
}

SerialInitOptions MySerial::ParseOptions(const nlohmann::json& cfg) {
    if (!cfg.is_object()) {
        throw std::invalid_argument("serial config must be a json object");
    }

    SerialInitOptions options;
    options.port = cfg.value("port", std::string());
    if (options.port.empty()) {
        throw std::invalid_argument("serial config missing non-empty field: port");
    }

    options.baudrate = cfg.value("baudrate", options.baudrate);
    options.timeout_ms = cfg.value("timeout_ms", options.timeout_ms);
    options.inter_byte_timeout_ms = cfg.value("inter_byte_timeout_ms", options.inter_byte_timeout_ms);
    options.read_timeout_constant_ms = cfg.value("read_timeout_constant_ms", options.timeout_ms);
    options.read_timeout_multiplier_ms = cfg.value("read_timeout_multiplier_ms", options.read_timeout_multiplier_ms);
    options.write_timeout_constant_ms = cfg.value("write_timeout_constant_ms", options.timeout_ms);
    options.write_timeout_multiplier_ms = cfg.value("write_timeout_multiplier_ms", options.write_timeout_multiplier_ms);
    options.bytesize = NormalizeToken(cfg.value("bytesize", options.bytesize));
    options.parity = NormalizeToken(cfg.value("parity", options.parity));
    options.stopbits = NormalizeToken(cfg.value("stopbits", options.stopbits));
    options.flowcontrol = NormalizeToken(cfg.value("flowcontrol", options.flowcontrol));
    options.auto_open = cfg.value("auto_open", options.auto_open);
    return options;
}

serial::bytesize_t MySerial::ParseBytesize(const std::string& value) {
    const std::string normalized = NormalizeToken(value);
    if (normalized == "fivebits" || normalized == "5") return serial::fivebits;
    if (normalized == "sixbits" || normalized == "6") return serial::sixbits;
    if (normalized == "sevenbits" || normalized == "7") return serial::sevenbits;
    if (normalized == "eightbits" || normalized == "8") return serial::eightbits;
    throw std::invalid_argument("unsupported bytesize: " + value);
}

serial::parity_t MySerial::ParseParity(const std::string& value) {
    const std::string normalized = NormalizeToken(value);
    if (normalized == "none" || normalized == "parity_none") return serial::parity_none;
    if (normalized == "odd" || normalized == "parity_odd") return serial::parity_odd;
    if (normalized == "even" || normalized == "parity_even") return serial::parity_even;
    if (normalized == "mark" || normalized == "parity_mark") return serial::parity_mark;
    if (normalized == "space" || normalized == "parity_space") return serial::parity_space;
    throw std::invalid_argument("unsupported parity: " + value);
}

serial::stopbits_t MySerial::ParseStopbits(const std::string& value) {
    const std::string normalized = NormalizeToken(value);
    if (normalized == "one" || normalized == "1") return serial::stopbits_one;
    if (normalized == "one_point_five" || normalized == "1.5") return serial::stopbits_one_point_five;
    if (normalized == "two" || normalized == "2") return serial::stopbits_two;
    throw std::invalid_argument("unsupported stopbits: " + value);
}

serial::flowcontrol_t MySerial::ParseFlowcontrol(const std::string& value) {
    const std::string normalized = NormalizeToken(value);
    if (normalized == "none" || normalized == "flowcontrol_none") return serial::flowcontrol_none;
    if (normalized == "software" || normalized == "flowcontrol_software") return serial::flowcontrol_software;
    if (normalized == "hardware" || normalized == "flowcontrol_hardware") return serial::flowcontrol_hardware;
    throw std::invalid_argument("unsupported flowcontrol: " + value);
}

std::string MySerial::ToString(serial::bytesize_t value) {
    switch (value) {
        case serial::fivebits: return "fivebits";
        case serial::sixbits: return "sixbits";
        case serial::sevenbits: return "sevenbits";
        case serial::eightbits: return "eightbits";
        default: return "unknown";
    }
}

std::string MySerial::ToString(serial::parity_t value) {
    switch (value) {
        case serial::parity_none: return "none";
        case serial::parity_odd: return "odd";
        case serial::parity_even: return "even";
        case serial::parity_mark: return "mark";
        case serial::parity_space: return "space";
        default: return "unknown";
    }
}

std::string MySerial::ToString(serial::stopbits_t value) {
    switch (value) {
        case serial::stopbits_one: return "one";
        case serial::stopbits_one_point_five: return "one_point_five";
        case serial::stopbits_two: return "two";
        default: return "unknown";
    }
}

std::string MySerial::ToString(serial::flowcontrol_t value) {
    switch (value) {
        case serial::flowcontrol_none: return "none";
        case serial::flowcontrol_software: return "software";
        case serial::flowcontrol_hardware: return "hardware";
        default: return "unknown";
    }
}

} // namespace my_serial