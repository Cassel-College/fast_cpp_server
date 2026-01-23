cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_DEVICE)
    message("BUILD_MY_DEVICE is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_device library..." COLOR yellow)

    set(MY_DEVICE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_device)
    file(GLOB_RECURSE MY_DEVICE_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_device/*.cpp)

    pretty_print_list("MY_DEVICE_INCLUDE_DIRECTORIES List" MY_DEVICE_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_DEVICE_SOURCES List" MY_DEVICE_SOURCES)

    add_library(my_device STATIC ${MY_DEVICE_SOURCES})
    target_include_directories(my_device PUBLIC ${MY_DEVICE_INCLUDE_DIRECTORIES})

    target_link_libraries(my_device PUBLIC pthread)
    target_link_libraries(my_device PUBLIC mylog)
    target_link_libraries(my_device PUBLIC myconfig)
    target_link_libraries(my_device PUBLIC my_data)
    target_link_libraries(my_device PUBLIC my_control)

    print_colored_message("Building my_device library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_DEVICE is OFF, skipping my_device library build")
    return()
endif()