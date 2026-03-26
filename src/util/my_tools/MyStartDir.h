#ifndef MY_START_DIR_H
#define MY_START_DIR_H
#include <unistd.h>
#include <limits.h>
#include <iostream>

namespace my_tools {

class MyStartDir {
public:
    MyStartDir();
    ~MyStartDir();
    std::string getStartDir() const { return start_dir_; }
private:
    std::string start_dir_;
};

}


#endif // MY_START_DIR_H

