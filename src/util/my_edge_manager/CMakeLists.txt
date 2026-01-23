cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_EDGE_MANAGER)
    message("BUILD_MY_EDGE_MANAGER is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_edge_manager library..." COLOR yellow)
    
    set(MY_EDGE_MANAGER_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_edge_manager)
    file(GLOB_RECURSE MY_EDGE_MANAGER_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_edge_manager/*.cpp)

    pretty_print_list("MY_EDGE_MANAGER_INCLUDE_DIRECTORIES List" MY_EDGE_MANAGER_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_EDGE_MANAGER_SOURCES List" MY_EDGE_MANAGER_SOURCES)

    add_library(my_edge_manager STATIC ${MY_EDGE_MANAGER_SOURCES})
    target_include_directories(my_edge_manager PUBLIC ${MY_EDGE_MANAGER_INCLUDE_DIRECTORIES})
    target_link_libraries(my_edge_manager PUBLIC pthread)
    target_link_libraries(my_edge_manager PUBLIC mylog)
    target_link_libraries(my_edge_manager PUBLIC myconfig)
    target_link_libraries(my_edge_manager PUBLIC my_network)
    print_colored_message("Building my_edge_manager library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_EDGE_MANAGER is OFF, skipping my_edge_manager library build")
    return()
endif()

