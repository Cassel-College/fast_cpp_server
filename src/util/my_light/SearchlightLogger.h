#ifndef SEARCHLIGHT_LOGGER_H
#define SEARCHLIGHT_LOGGER_H

#include <string>

namespace SearchlightControl {

// 统一日志入口：所有调试、状态变化和异常路径都通过该函数输出。
void print_log(std::string message);

}  // namespace SearchlightControl

#endif  // SEARCHLIGHT_LOGGER_H
