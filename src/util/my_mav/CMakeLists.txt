cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_MAV)
    message("BUILD_MY_MAV is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_mav library..." COLOR yellow)
    
    set(MY_MAV_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_mav)
    file(GLOB_RECURSE MY_MAV_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_mav/*.cpp)

    pretty_print_list("MY_MAV_INCLUDE_DIRECTORIES List" MY_MAV_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_MAV_SOURCES List" MY_MAV_SOURCES)

    add_library(my_mav STATIC ${MY_MAV_SOURCES})
    target_include_directories(my_mav PUBLIC ${MY_MAV_INCLUDE_DIRECTORIES})
    target_link_libraries(my_mav PUBLIC pthread)
    target_link_libraries(my_mav PUBLIC mylog)
    target_link_libraries(my_mav PUBLIC my_mavsdk)
    print_colored_message("Building my_mav library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_MAV is OFF, skipping my_mav library build")
    return()
endif()