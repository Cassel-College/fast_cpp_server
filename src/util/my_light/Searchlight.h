#ifndef SEARCHLIGHT_H
#define SEARCHLIGHT_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "SearchlightCommandBuilder.h"
#include "SearchlightConfig.h"
#include "SearchlightLedController.h"
#include "SearchlightServoController.h"
#include "SearchlightState.h"
#include "SerialPort.h"

namespace SearchlightControl {

class LightTask {
public:
    LightTask();
    LightTask(const std::string& command_name, const std::vector<uint8_t>& frame_bytes);

    std::string command;
    std::vector<uint8_t> parameters;
};

class Searchlight {
public:
    Searchlight();
    ~Searchlight();

    void initialize(const SearchlightInitConfig& config);
    bool online();
    bool setting_light_level(int level);
    bool setting_flash(int hz);
    bool open_flash();
    bool close_flash();
    bool home();
    bool move_to(int x, int y);
    bool open_led();
    bool close_led();
    void get_status(std::string& status);
    bool start();
    void shutdown();

private:
    bool enqueueCommand(const std::string& command, const std::vector<uint8_t>& frame);
    bool ensureControlReady(const std::string& command);
    void readSerialLoop();
    void workerLoop();
    void waitInitialDeviceFrame();

    SearchlightInitConfig config_;
    std::unique_ptr<SerialPort> serial_;
    SearchlightCommandBuilder builder_;
    SearchlightLedController led_controller_;
    SearchlightServoController servo_controller_;
    SearchlightState state_;

    std::atomic<bool> running_;
    std::mutex queue_mutex_;
    std::mutex build_mutex_;
    std::condition_variable queue_cv_;
    std::deque<LightTask> task_queue_;

    std::thread read_serial;
    std::thread worker_thread;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_H
