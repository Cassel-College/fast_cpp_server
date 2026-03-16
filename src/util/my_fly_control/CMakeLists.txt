cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_FLY_CONTROL)
    message("BUILD_MY_FLY_CONTROL is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_fly_control library..." COLOR yellow)

    set(MY_FLY_CONTROL_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_fly_control)
    file(GLOB_RECURSE MY_FLY_CONTROL_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_fly_control/*.cpp)

    pretty_print_list("MY_FLY_CONTROL_INCLUDE_DIRECTORIES List" MY_FLY_CONTROL_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_FLY_CONTROL_SOURCES List" MY_FLY_CONTROL_SOURCES)

    add_library(my_fly_control STATIC ${MY_FLY_CONTROL_SOURCES})
    target_include_directories(my_fly_control PUBLIC ${MY_FLY_CONTROL_INCLUDE_DIRECTORIES})

    target_link_libraries(my_fly_control PUBLIC pthread)
    target_link_libraries(my_fly_control PUBLIC mylog)
    target_link_libraries(my_fly_control PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(my_fly_control PUBLIC my_serial)
    target_link_libraries(my_fly_control PUBLIC my_serial_core)
    target_link_libraries(my_fly_control PUBLIC my_tools)

    print_colored_message("Building my_fly_control library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_FLY_CONTROL is OFF, skipping my_fly_control library build")
    return()
endif()
