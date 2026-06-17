#ifndef SEARCHLIGHT_SERIAL_PORT_H
#define SEARCHLIGHT_SERIAL_PORT_H

#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace SearchlightControl {

// 串口通信类：封装 POSIX UART 打开、配置、读写和关闭。
class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    bool openPort(const std::string& device, int baud_rate);
    void closePort();
    bool isOpen() const;
    ssize_t readBytes(uint8_t* buffer, size_t size, int timeout_ms);
    bool writeBytes(const std::vector<uint8_t>& bytes, std::string& error);
    std::string device() const;

private:
    bool configure(int baud_rate);
    static bool baudToTermios(int baud_rate, speed_t& speed);

    int fd_;
    std::string device_;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_SERIAL_PORT_H
