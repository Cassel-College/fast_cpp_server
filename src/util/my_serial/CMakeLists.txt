cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_SERIAL)
    message("BUILD_MY_SERIAL is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_serial library..." COLOR yellow)

    set(MY_SERIAL_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_serial)
    file(GLOB_RECURSE MY_SERIAL_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_serial/*.cpp)

    pretty_print_list("MY_SERIAL_INCLUDE_DIRECTORIES List" MY_SERIAL_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_SERIAL_SOURCES List" MY_SERIAL_SOURCES)

    add_library(my_serial STATIC ${MY_SERIAL_SOURCES})
    target_include_directories(my_serial PUBLIC ${MY_SERIAL_INCLUDE_DIRECTORIES})

    target_link_libraries(my_serial PUBLIC pthread)
    target_link_libraries(my_serial PUBLIC mylog)
    target_link_libraries(my_serial PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(my_serial PUBLIC my_serial_core)

    print_colored_message("Building my_serial library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_SERIAL is OFF, skipping my_serial library build")
    return()
endif()