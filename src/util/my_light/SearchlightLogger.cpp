#include "SearchlightLogger.h"
#include "MyLog.h"

namespace SearchlightControl {

void print_log(std::string message) {
    MYLOG_INFO("[SearchlightManager] {}", message);
}

}  // namespace SearchlightControl
