#include "MyStartDir.h"
#include "MyLog.h"

    
namespace my_tools {

MyStartDir::MyStartDir() {
    char buffer[PATH_MAX];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        start_dir_ = buffer;
        MYLOG_INFO("[MyStartDir] 程序启动目录: {}", start_dir_);
    } else {
        MYLOG_ERROR("Error getting current working directory");
    }
}

MyStartDir::~MyStartDir() = default;

} // namespace my_tools
