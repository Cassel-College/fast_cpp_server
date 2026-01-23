cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_DATA)
    message("BUILD_MY_DATA is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_data library..." COLOR yellow)
    
    set(MY_DATA_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_data)
    file(GLOB_RECURSE MY_DATA_SOURCES 
            ${PROJECT_SOURCE_DIR}/src/util/my_data/*.cpp
    )

    pretty_print_list("MY_DATA_INCLUDE_DIRECTORIES List" MY_DATA_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_DATA_SOURCES List" MY_DATA_SOURCES)

    add_library(my_data STATIC ${MY_DATA_SOURCES})
    target_include_directories(my_data PUBLIC ${MY_DATA_INCLUDE_DIRECTORIES})
    target_link_libraries(my_data PUBLIC pthread)
    target_link_libraries(my_data PUBLIC mylog)
    target_link_libraries(my_data PUBLIC myconfig)
    print_colored_message("Building my_data library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_DATA is OFF, skipping my_data library build")
    return()
endif()

