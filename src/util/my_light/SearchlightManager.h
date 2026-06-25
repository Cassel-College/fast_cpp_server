#ifndef SEARCHLIGHT_MANAGER_H
#define SEARCHLIGHT_MANAGER_H

#include <string>

#include "Searchlight.h"

namespace SearchlightControl {

class SearchlightManager {
public:
    static SearchlightManager& getInstance();

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

    Searchlight& device();

private:
    SearchlightManager();
    SearchlightManager(const SearchlightManager&) = delete;
    SearchlightManager& operator=(const SearchlightManager&) = delete;

    Searchlight searchlight_;
};

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_MANAGER_H
