cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_LIGHT)
    message("BUILD_MY_LIGHT is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_light library..." COLOR yellow)
    
    set(MYLIGHT_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_light)
    file(GLOB_RECURSE MYLIGHT_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_light/*.cpp)

    pretty_print_list("MYLIGHT_INCLUDE_DIRECTORIES List" MYLIGHT_INCLUDE_DIRECTORIES)
    pretty_print_list("MYLIGHT_SOURCES List" MYLIGHT_SOURCES)

    add_library(my_light STATIC ${MYLIGHT_SOURCES})
    target_include_directories(my_light PUBLIC ${MYLIGHT_INCLUDE_DIRECTORIES})
    target_link_libraries(my_light PUBLIC pthread)
    target_link_libraries(my_light PUBLIC mylog)
    target_link_libraries(my_light PUBLIC myproto)
    target_link_libraries(my_light PUBLIC myconfig)
    print_colored_message("Building my_light library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_LIGHT is OFF, skipping my_light library build")
    return()
endif()

