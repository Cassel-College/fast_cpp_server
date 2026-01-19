# cmake/deps/setup_gtest.cmake

# 防止 GTest/GMock 也就是 make install 的时候把自己安装到系统目录
set(INSTALL_GTEST OFF CACHE BOOL "Disable GTest installation" FORCE)
set(INSTALL_GMOCK OFF CACHE BOOL "Disable GMock installation" FORCE)

# 某些旧版本或者特殊环境下，可能需要强制使用多线程
if(NOT WIN32)
    set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)
endif()

# 引入项目
add_subdirectory(external/googletest EXCLUDE_FROM_ALL)

# 导出 GTest 的包含路径（以便某些特殊模块可能需要 include <gtest/gtest.h>）
# 虽然通常 target_link_libraries 会自动处理，但显式列出比较保险
list(APPEND THIRD_PARTY_INCLUDES 
    ${CMAKE_SOURCE_DIR}/external/googletest/googletest/include
    ${CMAKE_SOURCE_DIR}/external/googletest/googlemock/include
)