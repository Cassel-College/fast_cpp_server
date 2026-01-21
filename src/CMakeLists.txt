cmake_minimum_required(VERSION 3.10)

# 从源文件列表中移除 main.cpp
list(REMOVE_ITEM SOURCES ${PROJECT_SOURCE_DIR}/src/main.cpp)

# 查找所有通用源文件，不包含 main.cpp
file(GLOB_RECURSE MYLIB_SOURCES
    ${PROJECT_SOURCE_DIR}/src/tools/simple_tcp_client/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/thread_safe_buffer/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ping_tools/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/example/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/thread_safe_queue/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/service_guard/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/init_tools/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/free_func/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/pipeline/*.cpp
)


set(BUILD_MY_LOG            ON CACHE BOOL "Build mylog library")
set(BUILD_MY_CONFIG         ON CACHE BOOL "Build my_config library")
set(BUILD_MY_PROTO          ON CACHE BOOL "Build myproto library")
set(BUILD_MY_ARG_PARSER     ON CACHE BOOL "Build my_arg_parser library")
set(BUILD_MY_SOFT_HEALTHY   ON CACHE BOOL "Build my_soft_healthy library")
set(BUILD_MY_SYSTEM_HEALTHY ON CACHE BOOL "Build my_system_healthy library")
set(BUILD_MY_HEARTBEAT      ON CACHE BOOL "Build my_heartbeat library")
set(BUILD_MY_CACHE          ON CACHE BOOL "Build my_cache library")
set(BUILD_MY_DB             ON CACHE BOOL "Build my_db library")
set(BUILD_MY_API            ON CACHE BOOL "Build my_api library")
set(BUILD_MY_EDGE           ON CACHE BOOL "Build my_edge library")
set(BUILD_MY_DATA           ON CACHE BOOL "Build my_data library")
set(BUILD_MY_CONTROL        ON CACHE BOOL "Build my_control library")
set(BUILD_MY_COMM           ON CACHE BOOL "Build my_comm library")
set(BUILD_MY_AI_INTERFACE   ON CACHE BOOL "Build my_ai_interface library")
set(BUILD_MY_NETWORK        ON CACHE BOOL "Build my_network library")
set(BUILD_MY_DOCTOR         ON CACHE BOOL "Build my_doctor library")
set(BUILD_MY_MQTT           ON CACHE BOOL "Build my_mqtt library")

add_subdirectory(util/my_log)
add_subdirectory(util/my_config)
add_subdirectory(protobuf)
add_subdirectory(util/my_arg_parser)
add_subdirectory(util/my_soft_healthy)
add_subdirectory(util/my_system_healthy)
add_subdirectory(util/my_heartbeat)
add_subdirectory(util/my_cache)
add_subdirectory(util/my_db)
add_subdirectory(util/my_api)
add_subdirectory(util/my_edge)
add_subdirectory(util/my_data)
add_subdirectory(util/my_doctor)
add_subdirectory(util/my_control)
add_subdirectory(util/my_comm)
add_subdirectory(util/my_ai_interface)
add_subdirectory(util/my_network)
add_subdirectory(util/my_mqtt)

# 创建静态库 mylib
add_library(mylib STATIC ${MYLIB_SOURCES})

set(MYLIB_INCLUDE_DIRECTORIES
    ${PROJECT_SOURCE_DIR}/src/tools/simple_tcp_client
    ${PROJECT_SOURCE_DIR}/src/tools/thread_safe_buffer
    ${PROJECT_SOURCE_DIR}/src/tools/ping_tools
    ${PROJECT_SOURCE_DIR}/src/tools/example
    ${PROJECT_SOURCE_DIR}/src/tools/thread_safe_queue
    ${PROJECT_SOURCE_DIR}/src/tools/service_guard
    ${PROJECT_SOURCE_DIR}/src/tools/init_tools
    ${PROJECT_SOURCE_DIR}/src/tools/free_func
    ${PROJECT_SOURCE_DIR}/src/tools/pipeline
)

# 头文件路径
target_include_directories(mylib PUBLIC ${MYLIB_INCLUDE_DIRECTORIES})

# 如果你有条件判断 ENABLE_COVERAGE，可以在此处添加
if (ENABLE_COVERAGE)
    target_compile_options(mylib PRIVATE --coverage)
    target_link_libraries(mylib PUBLIC --coverage)
endif()

# 链接其他外部库
# 在根 CMakeLists.txt 中已经 add_subdirectory
print_colored_message("------------------------------" COLOR magenta)
print_colored_message("Adding mylib library with sources ..." COLOR yellow)
pretty_print_list("MYLIB_SOURCES List" MYLIB_SOURCES)
pretty_print_list("MYLIB_INCLUDE_DIRECTORIES List" MYLIB_INCLUDE_DIRECTORIES)

# target_link_libraries(mylib PUBLIC spdlog)
target_link_libraries(mylib PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(mylib PUBLIC protobuf::libprotobuf)
target_link_libraries(mylib PUBLIC oatpp::oatpp)
target_link_libraries(mylib PUBLIC oatpp::oatpp-swagger)
target_link_libraries(mylib PUBLIC mylog)
target_link_libraries(mylib PUBLIC myconfig)
target_link_libraries(mylib PUBLIC my_heartbeat)
target_link_libraries(mylib PUBLIC my_edge)
target_link_libraries(mylib PUBLIC my_db)
target_link_libraries(mylib PUBLIC my_cache)
target_link_libraries(mylib PUBLIC my_comm)
target_link_libraries(mylib PUBLIC my_control)
target_link_libraries(mylib PUBLIC my_network)
target_link_libraries(mylib PUBLIC my_ai_interface)
target_link_libraries(mylib PUBLIC my_data)
target_link_libraries(mylib PUBLIC my_api)
target_link_libraries(mylib PUBLIC my_mqtt)
target_link_libraries(mylib PUBLIC myproto)
target_link_libraries(mylib PUBLIC cpr::cpr)
target_link_libraries(mylib PUBLIC libzmq)
target_link_libraries(mylib PUBLIC pthread)
target_link_libraries(mylib PRIVATE my_soft_healthy)
target_link_libraries(mylib PRIVATE my_system_healthy)
target_link_libraries(mylib PRIVATE symengine)

print_colored_message("Adding mylib library with sources over" COLOR yellow)
print_colored_message("------------------------------" COLOR magenta)