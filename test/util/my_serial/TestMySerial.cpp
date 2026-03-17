#include "gtest/gtest.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>

#if defined(__linux__) || defined(__APPLE__)
#include <pty.h>
#endif

#include <nlohmann/json.hpp>

#include "MySerial.h"

namespace {

class PtyPair {
public:
    PtyPair() {
#if defined(__linux__) || defined(__APPLE__)
        char slave_name[256] = {0};
        if (openpty(&master_fd_, &slave_fd_, slave_name, nullptr, nullptr) != 0) {
            throw std::runtime_error(std::string("openpty failed: ") + std::strerror(errno));
        }
        slave_path_ = slave_name;
#else
        throw std::runtime_error("pty test is only supported on unix-like systems");
#endif
    }

    ~PtyPair() {
        if (master_fd_ >= 0) {
            ::close(master_fd_);
        }
        if (slave_fd_ >= 0) {
            ::close(slave_fd_);
        }
    }

    int master_fd() const { return master_fd_; }
    const std::string& slave_path() const { return slave_path_; }

private:
    int master_fd_{-1};
    int slave_fd_{-1};
    std::string slave_path_;
};

} // namespace

TEST(MySerialTest, InitRequiresPort) {
    my_serial::MySerial serial_tool;
    std::string err;

    const bool ok = serial_tool.Init(nlohmann::json::object(), &err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
}

TEST(MySerialTest, ListAvailablePortsReturnsJsonArrayEntries) {
    my_serial::MySerial serial_tool;
    const auto ports = serial_tool.ListAvailablePorts();
    for (const auto& item : ports) {
        EXPECT_TRUE(item.contains("port"));
        EXPECT_TRUE(item.contains("description"));
        EXPECT_TRUE(item.contains("hardware_id"));
    }
}

TEST(MySerialTest, InitOpenWriteReadAndSelfCheckWithPty) {
#if defined(__linux__) || defined(__APPLE__)
    PtyPair pty_pair;
    my_serial::MySerial serial_tool;

    nlohmann::json cfg = {
        {"port", pty_pair.slave_path()},
        {"baudrate", 115200},
        {"timeout_ms", 500},
        {"auto_open", true}
    };

    std::string err;
    ASSERT_TRUE(serial_tool.Init(cfg, &err)) << err;
    EXPECT_TRUE(serial_tool.IsOpen());

    const auto self_check = serial_tool.SelfCheck();
    EXPECT_TRUE(self_check.detail.contains("snapshot"));
    EXPECT_TRUE(self_check.detail.contains("available_ports"));

    const std::string outbound = "ping-from-myserial";
    ASSERT_EQ(serial_tool.Write(outbound, &err), outbound.size()) << err;

    char read_buf[128] = {0};
    const ssize_t master_read_size = ::read(pty_pair.master_fd(), read_buf, sizeof(read_buf));
    ASSERT_GT(master_read_size, 0);
    EXPECT_EQ(std::string(read_buf, static_cast<size_t>(master_read_size)), outbound);

    const std::string inbound = "pong-from-master";
    ASSERT_EQ(::write(pty_pair.master_fd(), inbound.data(), inbound.size()), static_cast<ssize_t>(inbound.size()));

    const std::string received = serial_tool.Read(inbound.size(), &err);
    EXPECT_EQ(received, inbound) << err;

    const auto snapshot = serial_tool.GetSnapshot();
    EXPECT_TRUE(snapshot.initialized);
    EXPECT_TRUE(snapshot.open);
    EXPECT_EQ(snapshot.port, pty_pair.slave_path());
    EXPECT_EQ(snapshot.baudrate, 115200u);
#else
    GTEST_SKIP() << "pty-based serial test only runs on unix-like systems";
#endif
}

TEST(MySerialTest, InitSupportsDeviceStyleSerialConfigWithPty) {
#if defined(__linux__) || defined(__APPLE__)
    PtyPair pty_pair;
    my_serial::MySerial serial_tool;

    nlohmann::json cfg = {
        {"id", "serial0"},
        {"enabled", true},
        {"device", pty_pair.slave_path()},
        {"baud_rate", 115200},
        {"data_bits", 8},
        {"stop_bits", 1},
        {"parity", "none"},
        {"flow_control", "none"},
        {"read_mode", "blocking"},
        {"timeout_ms", 500},
        {"vmin", 0},
        {"vtime", 10},
        {"raw_mode", true},
        {"dtr_rts", "auto"},
        {"mtu", 512},
        {"description", "飞控串口，连接外设飞控，用于模块间消息通信"}
    };

    std::string err;
    ASSERT_TRUE(serial_tool.Init(cfg, &err)) << err;
    EXPECT_TRUE(serial_tool.IsOpen());

    const auto snapshot = serial_tool.GetSnapshot();
    EXPECT_TRUE(snapshot.initialized);
    EXPECT_TRUE(snapshot.open);
    EXPECT_EQ(snapshot.port, pty_pair.slave_path());
    EXPECT_EQ(snapshot.baudrate, 115200u);
    EXPECT_EQ(snapshot.bytesize, "eightbits");
    EXPECT_EQ(snapshot.parity, "none");
    EXPECT_EQ(snapshot.stopbits, "one");
    EXPECT_EQ(snapshot.flowcontrol, "none");
#else
    GTEST_SKIP() << "pty-based serial test only runs on unix-like systems";
#endif
}

TEST(MySerialTest, SelfTestLoopbackCanRoundTripWithPty) {
#if defined(__linux__) || defined(__APPLE__)
    PtyPair pty_pair;
    my_serial::MySerial serial_tool;

    nlohmann::json cfg = {
        {"port", pty_pair.slave_path()},
        {"baudrate", 115200},
        {"timeout_ms", 500},
        {"auto_open", true}
    };

    std::string err;
    ASSERT_TRUE(serial_tool.Init(cfg, &err)) << err;

    const std::string payload = "loopback-payload";
    std::thread echo_thread([&pty_pair, &payload]() {
        char buffer[128] = {0};
        const ssize_t read_size = ::read(pty_pair.master_fd(), buffer, sizeof(buffer));
        if (read_size > 0) {
            ::write(pty_pair.master_fd(), buffer, static_cast<size_t>(read_size));
        }
    });

    const auto result = serial_tool.SelfTestLoopback(payload);
    echo_thread.join();

    EXPECT_TRUE(result.success) << result.summary;
    EXPECT_EQ(result.detail.value("received", std::string()), payload);
#else
    GTEST_SKIP() << "pty-based serial test only runs on unix-like systems";
#endif
}