cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_API)
    message("BUILD_MY_API is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_api library..." COLOR yellow)
    
    set(MY_API_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_api)
    file(GLOB_RECURSE MY_API_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_api/*.cpp)

    pretty_print_list("MYCONFIG_INCLUDE_DIRECTORIES List" MY_API_INCLUDE_DIRECTORIES)
    pretty_print_list("MYCONFIG_SOURCES List" MY_API_SOURCES)

    add_library(my_api STATIC ${MY_API_SOURCES})
    target_include_directories(my_api PUBLIC ${MY_API_INCLUDE_DIRECTORIES})
    target_link_libraries(my_api PUBLIC mylog)
    target_link_libraries(my_api PUBLIC myconfig)
    target_link_libraries(my_api PUBLIC my_edge)
    target_link_libraries(my_api PUBLIC my_heartbeat)
    target_link_libraries(my_api PUBLIC my_edge_manager)
    target_link_libraries(my_api PUBLIC oatpp::oatpp)
    target_link_libraries(my_api PUBLIC oatpp::oatpp-swagger)
    target_link_libraries(my_api PUBLIC nlohmann_json::nlohmann_json)
    print_colored_message("Building my_api library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_API is OFF, skipping my_config library build")
    return()
endif()

