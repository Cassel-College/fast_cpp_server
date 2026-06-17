#include "SearchlightManager.h"

namespace SearchlightControl {

SearchlightManager::SearchlightManager() {}

SearchlightManager& SearchlightManager::getInstance() {
    static SearchlightManager instance;
    return instance;
}

void SearchlightManager::initialize(const SearchlightInitConfig& config) {
    searchlight_.initialize(config);
}

bool SearchlightManager::start() {
    return searchlight_.start();
}

bool SearchlightManager::online() {
    return searchlight_.online();
}

bool SearchlightManager::setting_light_level(int level) {
    return searchlight_.setting_light_level(level);
}

bool SearchlightManager::setting_flash(int hz) {
    return searchlight_.setting_flash(hz);
}

bool SearchlightManager::open_flash() {
    return searchlight_.open_flash();
}

bool SearchlightManager::close_flash() {
    return searchlight_.close_flash();
}

bool SearchlightManager::home() {
    return searchlight_.home();
}

bool SearchlightManager::move_to(int x, int y) {
    return searchlight_.move_to(x, y);
}

bool SearchlightManager::open_led() {
    return searchlight_.open_led();
}

bool SearchlightManager::close_led() {
    return searchlight_.close_led();
}

void SearchlightManager::get_status(std::string& status) {
    searchlight_.get_status(status);
}

void SearchlightManager::shutdown() {
    searchlight_.shutdown();
}

Searchlight& SearchlightManager::device() {
    return searchlight_;
}

}  // namespace SearchlightControl
