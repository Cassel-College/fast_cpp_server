cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_DB)
    message("BUILD_MY_DB is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_db library..." COLOR yellow)

    set(MY_DB_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_db)
    file(GLOB_RECURSE MY_DB_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_db/*.cpp)

    pretty_print_list("MY_DB_INCLUDE_DIRECTORIES List" MY_DB_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_DB_SOURCES List" MY_DB_SOURCES)

    add_library(my_db STATIC ${MY_DB_SOURCES})
    target_include_directories(my_db PUBLIC ${MY_DB_INCLUDE_DIRECTORIES})

    target_link_libraries(my_db PUBLIC pthread)
    target_link_libraries(my_db PUBLIC mylog)
    target_link_libraries(my_db PUBLIC myconfig)
    target_link_libraries(my_db PUBLIC my_data)

    # 关键：链接 sqlite amalgamation 静态库 target
    target_link_libraries(my_db PUBLIC sqlite3_static)

    print_colored_message("Building my_db library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_DB is OFF, skipping my_db library build")
    return()
endif()