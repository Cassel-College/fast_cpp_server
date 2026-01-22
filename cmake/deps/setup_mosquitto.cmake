# set(DOCUMENTATION OFF CACHE BOOL "Disable documentation generation")
# set(WITH_STATIC_LIBRARIES ON CACHE BOOL "" FORCE)
# # 开启共享库以供 CPack 识别，或者只用静态库（根据你的需求）
# set(WITH_SHARED_LIBRARIES ON CACHE BOOL "" FORCE) 
# add_subdirectory(external/mosquitto EXCLUDE_FROM_ALL)

set(WITH_STATIC_LIBRARIES   ON          CACHE BOOL "" FORCE) # 开启静态库编译 (.a)
set(WITH_SHARED_LIBRARIES   OFF         CACHE BOOL "" FORCE) # 如果你只需要静态库，可以关闭共享库
set(WITH_APPS               OFF         CACHE BOOL "" FORCE) # 不编译 mosquitto_pub/sub 等工具
set(WITH_BROKER             ON          CACHE BOOL "" FORCE) # 如果你只需要 Client 库，不编译服务端二进制
set(DOCUMENTATION           OFF         CACHE BOOL "Disable documentation generation")
set(DCMAKE_INSTALL_SBINDIR  "bin"       CACHE STRING    "Set sbin directory") # 设置 bin 目录

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/external/mosquitto/CMakeLists.txt")
    message(FATAL_ERROR "Mosquitto directory not found in external/mosquitto")
else()
    message(STATUS "添加 mosquitto 包含路径到 THIRD_PARTY_INCLUDES")
    message(STATUS "mosquitto include path: ${CMAKE_SOURCE_DIR}/external/mosquitto/include")
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/external/mosquitto/include")
        message(FATAL_ERROR "mosquitto include path not found: ${CMAKE_SOURCE_DIR}/external/mosquitto/include")
    else()
        print_colored_message("<include> Found mosquitto include: ${CMAKE_SOURCE_DIR}/external/mosquitto/include " COLOR green)
        list(APPEND THIRD_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/external/mosquitto/include)
        print_colored_message("<include>  Load mosquitto include path into THIRD_PARTY_INCLUDES" COLOR green)
    endif()
endif()
# 使用 EXCLUDE_FROM_ALL 可以防止在执行 'make install' 时把整个 mosquitto 装进系统目录
add_subdirectory(external/mosquitto EXCLUDE_FROM_ALL)