# set(DOCUMENTATION OFF CACHE BOOL "Disable documentation generation")
# set(WITH_STATIC_LIBRARIES ON CACHE BOOL "" FORCE)
# # 开启共享库以供 CPack 识别，或者只用静态库（根据你的需求）
# set(WITH_SHARED_LIBRARIES ON CACHE BOOL "" FORCE) 
# add_subdirectory(external/mosquitto EXCLUDE_FROM_ALL)

set(WITH_APPS               OFF         CACHE BOOL      "" FORCE)    # 不编译 mosquitto_pub ?等
set(WITH_STATIC_LIBRARIES   OFF         CACHE BOOL      "" FORCE)
set(INSTALL_HEADERS         OFF         CACHE BOOL      "" FORCE) # 尝试关闭头文件安装
set(DOCUMENTATION           OFF         CACHE BOOL      "Disable documentation generation")
set(DCMAKE_INSTALL_SBINDIR  "bin"       CACHE STRING    "Set sbin directory") # 设置 bin 目录

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/external/mosquitto/CMakeLists.txt")
    message(FATAL_ERROR "Mosquitto directory not found in external/mosquitto")
else()
    message(STATUS "添加 mosquitto 包含路径到 THIRD_PARTY_INCLUDES")
    message(STATUS "mosquitto include path: ${CMAKE_SOURCE_DIR}/external/mosquitto/include")
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/external/mosquitto/include")
        message(FATAL_ERROR "mosquitto include path not found: ${CMAKE_SOURCE_DIR}/external/mosquitto/include")
    else()
        print_colored_message("<include> Found mosquitto include" COLOR green)
        list(APPEND THIRD_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/external/mosquitto/include)
    endif()
endif()
add_subdirectory(external/mosquitto)