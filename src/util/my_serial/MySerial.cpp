#include "MySerial.h"

#include <algorithm>
#include <cctype>
#include <limits>
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

std::string SafeJsonDump(const nlohmann::json& value) {
    try {
        return value.dump();
    } catch (const std::exception& ex) {
        return std::string("<json dump failed: ") + ex.what() + ">";
    } catch (...) {
        return "<json dump failed: unknown exception>";
    }
}

std::string JsonValueToToken(const nlohmann::json& value, const char* field_name) {
    if (value.is_string()) {
        return NormalizeToken(value.get<std::string>());
    }
    if (value.is_number_unsigned()) {
        return std::to_string(value.get<std::uint64_t>());
    }
    if (value.is_number_integer()) {
        return std::to_string(value.get<std::int64_t>());
    }
    if (value.is_number_float()) {
        return NormalizeToken(value.dump());
    }
    throw std::invalid_argument(std::string("serial config field '") + field_name + "' must be a string or number");
}

std::string GetRequiredPortValue(const nlohmann::json& cfg) {
    for (const char* key : {"port", "device"}) {
        if (!cfg.contains(key) || cfg.at(key).is_null()) {
            continue;
        }

        if (!cfg.at(key).is_string()) {
            throw std::invalid_argument(std::string("serial config field '") + key + "' must be a string");
        }

        const std::string port = cfg.at(key).get<std::string>();
        if (!port.empty()) {
            return port;
        }
    }

    throw std::invalid_argument("serial config missing non-empty field: port/device");
}

uint32_t GetUint32Value(const nlohmann::json& cfg,
                       std::initializer_list<const char*> keys,
                       uint32_t default_value) {
    for (const char* key : keys) {
        if (!cfg.contains(key) || cfg.at(key).is_null()) {
            continue;
        }

        const auto& value = cfg.at(key);
        if (value.is_number_unsigned()) {
            const auto parsed = value.get<std::uint64_t>();
            if (parsed > static_cast<std::uint64_t>(std::numeric_limits<uint32_t>::max())) {
                throw std::invalid_argument(std::string("serial config field '") + key + "' is out of uint32 range");
            }
            return static_cast<uint32_t>(parsed);
        }

        if (value.is_number_integer()) {
            const auto parsed = value.get<std::int64_t>();
            if (parsed < 0 || parsed > static_cast<std::int64_t>(std::numeric_limits<uint32_t>::max())) {
                throw std::invalid_argument(std::string("serial config field '") + key + "' is out of uint32 range");
            }
            return static_cast<uint32_t>(parsed);
        }

        if (value.is_string()) {
            const auto parsed = std::stoull(value.get<std::string>());
            if (parsed > static_cast<unsigned long long>(std::numeric_limits<uint32_t>::max())) {
                throw std::invalid_argument(std::string("serial config field '") + key + "' is out of uint32 range");
            }
            return static_cast<uint32_t>(parsed);
        }

        throw std::invalid_argument(std::string("serial config field '") + key + "' must be an unsigned integer");
    }

    return default_value;
}

std::string GetTokenValue(const nlohmann::json& cfg,
                          std::initializer_list<const char*> keys,
                          const std::string& default_value) {
    for (const char* key : keys) {
        if (!cfg.contains(key) || cfg.at(key).is_null()) {
            continue;
        }
        return JsonValueToToken(cfg.at(key), key);
    }
    return NormalizeToken(default_value);
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

    const std::string cfg_dump = SafeJsonDump(cfg);
    const bool had_previous_serial = static_cast<bool>(serial_);
    const bool previous_open = serial_ && serial_->isOpen();
    const std::string previous_port = options_.port;

    SerialInitOptions parsed_options;
    bool options_parsed = false;
    std::string current_step = "start";

    try {
        MYLOG_INFO("[MySerial] Begin Init, cfg={}", cfg.dump(4));

        if (serial_ && serial_->isOpen()) {
            current_step = "close previous port";
            MYLOG_WARN("[MySerial] Closing previously opened port before re-init: {}", options_.port);
            serial_->close();
        }

        initialized_ = false;

        current_step = "parse config";
        parsed_options = ParseOptions(cfg);
        options_parsed = true;
        MYLOG_INFO(
            "[MySerial] Parsed config: port={}, baudrate={}, timeout_ms={}, auto_open={}, bytesize={}, parity={}, stopbits={}, flowcontrol={}, inter_byte_timeout_ms={}, read_timeout_constant_ms={}, read_timeout_multiplier_ms={}, write_timeout_constant_ms={}, write_timeout_multiplier_ms={}",
            parsed_options.port,
            parsed_options.baudrate,
            parsed_options.timeout_ms,
            parsed_options.auto_open ? "true" : "false",
            parsed_options.bytesize,
            parsed_options.parity,
            parsed_options.stopbits,
            parsed_options.flowcontrol,
            parsed_options.inter_byte_timeout_ms,
            parsed_options.read_timeout_constant_ms,
            parsed_options.read_timeout_multiplier_ms,
            parsed_options.write_timeout_constant_ms,
            parsed_options.write_timeout_multiplier_ms);

        current_step = "create serial instance";
        auto new_serial = std::make_unique<serial::Serial>();
        MYLOG_INFO("[MySerial] serial::Serial instance created, preparing low-level configuration");

        current_step = "apply basic port settings";
        new_serial->setPort(parsed_options.port);
        new_serial->setBaudrate(parsed_options.baudrate);

        current_step = "apply timeout settings";
        serial::Timeout timeout = BuildTimeout(parsed_options);
        new_serial->setTimeout(timeout);

        current_step = "apply line settings";
        new_serial->setBytesize(ParseBytesize(parsed_options.bytesize));
        new_serial->setParity(ParseParity(parsed_options.parity));
        new_serial->setStopbits(ParseStopbits(parsed_options.stopbits));
        new_serial->setFlowcontrol(ParseFlowcontrol(parsed_options.flowcontrol));

        if (parsed_options.auto_open) {
            current_step = "auto open port";
            MYLOG_INFO("[MySerial] auto_open=true, trying to open port: {}", parsed_options.port);
            new_serial->open();
            MYLOG_INFO("[MySerial] Port opened during Init: {}", parsed_options.port);
        } else {
            MYLOG_INFO("[MySerial] auto_open=false, skip opening port during Init: {}", parsed_options.port);
        }

        current_step = "commit initialized state";
        serial_ = std::move(new_serial);
        options_ = parsed_options;
        initialized_ = true;
        last_error_.clear();

        if (err != nullptr) {
            err->clear();
        }

        MYLOG_INFO("[MySerial] Init success: port={}, baudrate={}, auto_open={}, initialized=true",
                   options_.port,
                   options_.baudrate,
                   options_.auto_open ? "true" : "false");
        return true;
    } catch (const std::exception& ex) {
        initialized_ = false;
        serial_.reset();
        options_ = SerialInitOptions{};
        last_error_ = ex.what();
        if (err != nullptr) {
            *err = last_error_;
        }

        MYLOG_ERROR(
            "[MySerial] Init failed at step='{}', error='{}', cfg={}, had_previous_serial={}, previous_open={}, previous_port={}, parsed_ok={}, parsed_port={}, parsed_baudrate={}, parsed_auto_open={}",
            current_step,
            ex.what(),
            cfg_dump,
            had_previous_serial ? "true" : "false",
            previous_open ? "true" : "false",
            previous_port.empty() ? "<empty>" : previous_port,
            options_parsed ? "true" : "false",
            options_parsed ? parsed_options.port : std::string("<unparsed>"),
            options_parsed ? std::to_string(parsed_options.baudrate) : std::string("<unparsed>"),
            options_parsed ? (parsed_options.auto_open ? "true" : "false") : std::string("<unparsed>"));
        return false;
    } catch (...) {
        initialized_ = false;
        serial_.reset();
        options_ = SerialInitOptions{};
        last_error_ = "unknown exception";
        if (err != nullptr) {
            *err = last_error_;
        }

        MYLOG_ERROR(
            "[MySerial] Init failed at step='{}' with unknown exception, cfg={}, had_previous_serial={}, previous_open={}, previous_port={}, parsed_ok={}, parsed_port={}",
            current_step,
            cfg_dump,
            had_previous_serial ? "true" : "false",
            previous_open ? "true" : "false",
            previous_port.empty() ? "<empty>" : previous_port,
            options_parsed ? "true" : "false",
            options_parsed ? parsed_options.port : std::string("<unparsed>"));
        return false;
    }
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
    options.port = GetRequiredPortValue(cfg);
    options.baudrate = GetUint32Value(cfg, {"baudrate", "baud_rate"}, options.baudrate);
    options.timeout_ms = GetUint32Value(cfg, {"timeout_ms"}, options.timeout_ms);
    options.inter_byte_timeout_ms = GetUint32Value(cfg, {"inter_byte_timeout_ms"}, options.inter_byte_timeout_ms);
    options.read_timeout_constant_ms = GetUint32Value(cfg, {"read_timeout_constant_ms"}, options.timeout_ms);
    options.read_timeout_multiplier_ms = GetUint32Value(cfg, {"read_timeout_multiplier_ms"}, options.read_timeout_multiplier_ms);
    options.write_timeout_constant_ms = GetUint32Value(cfg, {"write_timeout_constant_ms"}, options.timeout_ms);
    options.write_timeout_multiplier_ms = GetUint32Value(cfg, {"write_timeout_multiplier_ms"}, options.write_timeout_multiplier_ms);
    options.bytesize = ToString(ParseBytesize(GetTokenValue(cfg, {"bytesize", "data_bits"}, options.bytesize)));
    options.parity = ToString(ParseParity(GetTokenValue(cfg, {"parity"}, options.parity)));
    options.stopbits = ToString(ParseStopbits(GetTokenValue(cfg, {"stopbits", "stop_bits"}, options.stopbits)));
    options.flowcontrol = ToString(ParseFlowcontrol(GetTokenValue(cfg, {"flowcontrol", "flow_control"}, options.flowcontrol)));
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