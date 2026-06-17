#include "SerialPort.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <vector>

#include "MyLog.h"
#include "SearchlightProtocol.h"

namespace SearchlightControl {

SerialPort::SerialPort() : fd_(-1) {}

SerialPort::~SerialPort() {
    closePort();
}

bool SerialPort::openPort(const std::string& device, int baud_rate) {
    closePort();

    MYLOG_INFO("串口：准备打开设备 " + device + "，波特率 " + std::to_string(baud_rate));
    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        MYLOG_INFO("串口：打开失败，原因：" + std::string(strerror(errno)));
        return false;
    }

    device_ = device;
    if (!configure(baud_rate)) {
        closePort();
        return false;
    }

    ::tcflush(fd_, TCIOFLUSH);
    MYLOG_INFO("串口：打开成功，已进入原始数据通信模式");
    return true;
}

void SerialPort::closePort() {
    if (fd_ >= 0) {
        MYLOG_INFO("串口：关闭设备 " + device_);
        ::close(fd_);
        fd_ = -1;
    }
}

bool SerialPort::isOpen() const {
    return fd_ >= 0;
}

ssize_t SerialPort::readBytes(uint8_t* buffer, size_t size, int timeout_ms) {
    if (fd_ < 0) {
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd_, &read_fds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    const int ready = ::select(fd_ + 1, &read_fds, NULL, NULL, &tv);
    if (ready < 0) {
        if (errno == EINTR) {
            return 0;
        }
        return -1;
    }
    if (ready == 0) {
        return 0;
    }

    const ssize_t n = ::read(fd_, buffer, size);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        return 0;
    }
    return n;
}

bool SerialPort::writeBytes(const std::vector<uint8_t>& bytes, std::string& error) {
    error.clear();
    if (fd_ < 0) {
        error = "串口未打开";
        return false;
    }

    size_t total = 0;
    while (total < bytes.size()) {
        const ssize_t n = ::write(fd_, &bytes[total], bytes.size() - total);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            error = strerror(errno);
            return false;
        }
        total += static_cast<size_t>(n);
    }

    ::tcdrain(fd_);
    MYLOG_INFO("串口：发送完成 " + std::to_string(bytes.size()) + " 字节，HEX=" +
              ProtocolCodec::toHex(bytes));
    return true;
}

std::string SerialPort::device() const {
    return device_;
}

bool SerialPort::configure(int baud_rate) {
    speed_t speed;
    if (!baudToTermios(baud_rate, speed)) {
        MYLOG_INFO("串口：不支持的波特率，仅允许 115200bps");
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (::tcgetattr(fd_, &tty) != 0) {
        MYLOG_INFO("串口：读取 termios 失败，原因：" + std::string(strerror(errno)));
        return false;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
    tty.c_cflag &= static_cast<tcflag_t>(~PARENB);
    tty.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
    tty.c_cflag &= static_cast<tcflag_t>(~CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (::tcsetattr(fd_, TCSANOW, &tty) != 0) {
        MYLOG_INFO("串口：配置 termios 失败，原因：" + std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool SerialPort::baudToTermios(int baud_rate, speed_t& speed) {
    if (baud_rate != 115200) {
        return false;
    }
    speed = B115200;
    return true;
}

}  // namespace SearchlightControl
