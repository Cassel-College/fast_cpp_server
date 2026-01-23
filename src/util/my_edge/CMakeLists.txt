cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_EDGE)
    message("BUILD_MY_EDGE is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_edge library..." COLOR yellow)
    
    set(MY_EDGE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_edge)
    file(GLOB_RECURSE MY_EDGE_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_edge/*.cpp)

    pretty_print_list("MY_EDGE_INCLUDE_DIRECTORIES List" MY_EDGE_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_EDGE_SOURCES List" MY_EDGE_SOURCES)

    add_library(my_edge STATIC ${MY_EDGE_SOURCES})
    target_include_directories(my_edge PUBLIC ${MY_EDGE_INCLUDE_DIRECTORIES})
    target_link_libraries(my_edge PUBLIC pthread)
    target_link_libraries(my_edge PUBLIC mylog)
    target_link_libraries(my_edge PUBLIC myconfig)
    target_link_libraries(my_edge PUBLIC my_data)
    target_link_libraries(my_edge PUBLIC my_control)
    target_link_libraries(my_edge PUBLIC my_device)
    target_link_libraries(my_edge PUBLIC my_network)
    target_link_libraries(my_edge PUBLIC my_db)
    print_colored_message("Building my_edge library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_EDGE is OFF, skipping my_edge library build")
    return()
endif()

