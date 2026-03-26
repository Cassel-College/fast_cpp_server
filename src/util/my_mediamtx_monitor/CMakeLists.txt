cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_MEDIAMTX_MONITOR)
    message("BUILD_MY_MEDIAMTX_MONITOR is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_mediamtx_monitor library..." COLOR yellow)

    set(MY_MEDIAMTX_MONITOR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_mediamtx_monitor)
    file(GLOB_RECURSE MY_MEDIAMTX_MONITOR_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_mediamtx_monitor/*.cpp)

    pretty_print_list("MY_MEDIAMTX_MONITOR_INCLUDE_DIRECTORIES List" MY_MEDIAMTX_MONITOR_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_MEDIAMTX_MONITOR_SOURCES List" MY_MEDIAMTX_MONITOR_SOURCES)

    add_library(my_mediamtx_monitor STATIC ${MY_MEDIAMTX_MONITOR_SOURCES})
    target_include_directories(my_mediamtx_monitor PUBLIC ${MY_MEDIAMTX_MONITOR_INCLUDE_DIRECTORIES})

    target_link_libraries(my_mediamtx_monitor PUBLIC pthread)
    target_link_libraries(my_mediamtx_monitor PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(my_mediamtx_monitor PUBLIC mylog)
    target_link_libraries(my_mediamtx_monitor PUBLIC my_tools)

    print_colored_message("Building my_mediamtx_monitor library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_MEDIAMTX_MONITOR is OFF, skipping my_mediamtx_monitor library build")
    return()
endif()
