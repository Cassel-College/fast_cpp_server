#include "SearchlightConfig.h"

namespace SearchlightControl {

SearchlightInitConfig::SearchlightInitConfig()
    // : driver("/dev/tty.usbserial-14220"),
    : driver("/dev/ttyUSB0"),
      baud_rate(115200),
      print_raw_serial(false),
      init_wait_ms(1200),
      offline_timeout_ms(2000) {}

    void SearchlightInitConfig::set_driver(const std::string& driver_path) {
        driver = driver_path;
    }

    std::string SearchlightInitConfig::get_driver() const {
        return driver;
    }

    void SearchlightInitConfig::set_baud_rate(int rate) {
        baud_rate = rate;
    }

    void SearchlightInitConfig::set_print_raw_serial(bool print) {
        print_raw_serial = print;
    }

    void SearchlightInitConfig::set_init_wait_ms(int wait_ms) {
        init_wait_ms = wait_ms;
    }

    void SearchlightInitConfig::set_offline_timeout_ms(int timeout_ms) {
        offline_timeout_ms = timeout_ms;
    }

    int SearchlightInitConfig::get_baud_rate() const {
        return baud_rate;
    }

    bool SearchlightInitConfig::get_print_raw_serial() const {
        return print_raw_serial;
    }

    int SearchlightInitConfig::get_init_wait_ms() const {
        return init_wait_ms;
    }

    int SearchlightInitConfig::get_offline_timeout_ms() const {
        return offline_timeout_ms;
    }

    std::string SearchlightInitConfig::GetConfigStr() const {                       // 获取当前配置
        return "Driver: " + driver +
               ", Baud Rate: " + std::to_string(baud_rate) +
               ", Print Raw Serial: " + (print_raw_serial ? "true" : "false") +
               ", Init Wait (ms): " + std::to_string(init_wait_ms) +
               ", Offline Timeout (ms): " + std::to_string(offline_timeout_ms);
    }

}  // namespace SearchlightControl
