cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_HEARTBEAT)
    message("BUILD_MY_HEARTBEAT is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_heartbeat library..." COLOR yellow)
    
    set(MY_HEARTBEAT_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_heartbeat)
    file(GLOB_RECURSE MY_HEARTBEAT_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_heartbeat/*.cpp)

    pretty_print_list("MY_HEARTBEAT_INCLUDE_DIRECTORIES List" MY_HEARTBEAT_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_HEARTBEAT_INCLUDE_SOURCES List" MY_HEARTBEAT_SOURCES)

    add_library(my_heartbeat STATIC ${MY_HEARTBEAT_SOURCES})
    target_include_directories(my_heartbeat PUBLIC ${MY_HEARTBEAT_INCLUDE_DIRECTORIES})
    target_link_libraries(my_heartbeat PUBLIC pthread)
    target_link_libraries(my_heartbeat PUBLIC mylog)
    target_link_libraries(my_heartbeat PUBLIC myconfig)
    target_link_libraries(my_heartbeat PUBLIC my_edge)
    target_link_libraries(my_heartbeat PUBLIC my_edge_manager)
    print_colored_message("Building my_heartbeat library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_HEARTBEAT is OFF, skipping my_heartbeat library build")
    return()
endif()

