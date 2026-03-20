cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_TOOLS)
    message("BUILD_MY_TOOLS is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_tools library..." COLOR yellow)

    set(MY_TOOLS_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_tools)
    file(GLOB_RECURSE MY_TOOLS_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_tools/*.cpp)

    pretty_print_list("MY_TOOLS_INCLUDE_DIRECTORIES List" MY_TOOLS_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_TOOLS_SOURCES List" MY_TOOLS_SOURCES)

    add_library(my_tools STATIC ${MY_TOOLS_SOURCES})
    target_include_directories(my_tools PUBLIC ${MY_TOOLS_INCLUDE_DIRECTORIES})

    target_link_libraries(my_tools PUBLIC pthread)
    target_link_libraries(my_tools PUBLIC cpr::cpr)
    target_link_libraries(my_tools PUBLIC mylog)
    target_link_libraries(my_tools PUBLIC myconfig)

    print_colored_message("Building my_tools library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_TOOLS is OFF, skipping my_tools library build")
    return()
endif()