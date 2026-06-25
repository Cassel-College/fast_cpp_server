#ifndef SEARCHLIGHT_CONFIG_H
#define SEARCHLIGHT_CONFIG_H

#include <string>

namespace SearchlightControl {

// 初始化参数类：当前协议只允许使用串口路径和固定波特率连接设备。
class SearchlightInitConfig {
public:
    SearchlightInitConfig();

    std::string driver;
    int baud_rate;
    bool print_raw_serial;
    int init_wait_ms;
    int offline_timeout_ms;

    void set_driver(const std::string& driver_path);        // 设置串口路径
    std::string get_driver() const;                         // 获取串口路径
    void set_baud_rate(int rate);                           // 设置波特率
    void set_print_raw_serial(bool print);                  // 设置是否打印原始串口数据
    void set_init_wait_ms(int wait_ms);                     // 设置初始化等待时间（毫秒）
    void set_offline_timeout_ms(int timeout_ms);            // 设置离线超时时间（毫秒）
    int get_baud_rate() const;                              // 获取波特率
    bool get_print_raw_serial() const;                      // 获取是否打印原始串口数据
    int get_init_wait_ms() const;                           // 获取初始化等待时间（毫秒）
    int get_offline_timeout_ms() const;                     // 获取离线超时时间（毫秒）
    std::string GetConfigStr() const;                       // 获取当前配置的字符串表示
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_CONFIG_H
