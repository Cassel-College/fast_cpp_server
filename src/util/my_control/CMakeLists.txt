cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_CONTROL)
    message("BUILD_MY_CONTROL is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_control library..." COLOR yellow)

    set(MY_CONTROL_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_control)
    file(GLOB_RECURSE MY_CONTROL_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_control/*.cpp)

    pretty_print_list("MY_CONTROL_INCLUDE_DIRECTORIES List" MY_CONTROL_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_CONTROL_SOURCES List" MY_CONTROL_SOURCES)

    add_library(my_control STATIC ${MY_CONTROL_SOURCES})
    target_include_directories(my_control PUBLIC ${MY_CONTROL_INCLUDE_DIRECTORIES})

    target_link_libraries(my_control PUBLIC pthread)
    target_link_libraries(my_control PUBLIC mylog)
    target_link_libraries(my_control PUBLIC myconfig)
    target_link_libraries(my_control PUBLIC my_data)

    print_colored_message("Building my_control library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_CONTROL is OFF, skipping my_control library build")
    return()
endif()