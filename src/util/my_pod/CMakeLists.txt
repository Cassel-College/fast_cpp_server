cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_POD)
    message("BUILD_MY_POD is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_pod library..." COLOR yellow)

    # ========== 头文件目录 ==========
    set(MY_POD_INCLUDE_DIRECTORIES
        ${PROJECT_SOURCE_DIR}/src/util/my_pod
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/common
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/pod/interface
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/pod/base
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/pod/dji
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/pod/pinling
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/capability/interface
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/capability/base
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/capability/dji
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/capability/pinling
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/session/interface
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/session/base
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/session/dji
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/session/pinling
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/registry
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/manager
        ${PROJECT_SOURCE_DIR}/src/util/my_pod/utils
    )

    # ========== 源文件收集 ==========
    file(GLOB_RECURSE MY_POD_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_pod/*.cpp)

    pretty_print_list("MY_POD_INCLUDE_DIRECTORIES List" MY_POD_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_POD_SOURCES List" MY_POD_SOURCES)

    # ========== 构建静态库 ==========
    add_library(my_pod STATIC ${MY_POD_SOURCES})
    target_include_directories(my_pod PUBLIC ${MY_POD_INCLUDE_DIRECTORIES})
    target_link_libraries(my_pod PUBLIC mylog)

    print_colored_message("Building my_pod library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_POD is OFF, skipping my_pod library build")
    return()
endif()
