cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

# 项目信息
project(fast_cpp_server)

message("PROJECT_NAME: ${PROJECT_NAME}")

message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 全局禁用 warning
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# 启用覆盖率
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

option(ENABLE_COVERAGE "Enable coverage reporting" ON)

if (ENABLE_COVERAGE)
    message(STATUS "Coverage enabled")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# 设置静态库和可执行文件的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # 静态库 (.a)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # 动态库 (.so)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # 可执行文件

# 2. 加载模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(utils)

set(THIRD_INCLUDE_DIRECTORIES "")

message(STATUS ">>> 正在加载第三方依赖...")
include(cmake/setup_dependencies.cmake)

pretty_print_list("THIRD_INCLUDE_DIRECTORIES List" THIRD_INCLUDE_DIRECTORIES)

add_subdirectory(src)

set(MY_INCLUDE_DIRECTORIES "")
set(MIAN_SOURCE "${PROJECT_SOURCE_DIR}/src/main.cpp")

# 查找所有源文件
file(GLOB_RECURSE SOURCES "")

include_directories(
    ${THIRD_INCLUDE_DIRECTORIES}
)

print_colored_message("------------------------------" COLOR magenta)
print_colored_message("Building bin exec file ..." COLOR yellow)
# 添加可执行文件
add_executable(${PROJECT_NAME} ${MIAN_SOURCE} ${SOURCES})

# # 链接静态库，使用 PRIVATE 关键字
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE mylog)
target_link_libraries(${PROJECT_NAME} PRIVATE myconfig)
target_link_libraries(${PROJECT_NAME} PRIVATE myproto)
target_link_libraries(${PROJECT_NAME} PRIVATE my_arg_parser)
target_link_libraries(${PROJECT_NAME} PRIVATE my_soft_healthy)
target_link_libraries(${PROJECT_NAME} PRIVATE my_system_healthy)
target_link_libraries(${PROJECT_NAME} PRIVATE my_heartbeat)
target_link_libraries(${PROJECT_NAME} PRIVATE my_edge)
target_link_libraries(${PROJECT_NAME} PRIVATE my_db)
target_link_libraries(${PROJECT_NAME} PRIVATE my_doctor)
target_link_libraries(${PROJECT_NAME} PRIVATE my_cache)
target_link_libraries(${PROJECT_NAME} PRIVATE mylib)
target_link_libraries(${PROJECT_NAME} PRIVATE libmosquitto_static)
# 链接 OpenCV 库
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_calib3d)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_core)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_dnn)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_features2d)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_flann)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_gapi)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_highgui)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_imgcodecs)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_imgproc)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_ml)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_objdetect)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_photo)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_stitching)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_video)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_videoio)
target_link_libraries(${PROJECT_NAME} PRIVATE oatpp::oatpp)
target_link_libraries(${PROJECT_NAME} PRIVATE oatpp::oatpp-swagger)
target_link_libraries(${PROJECT_NAME} PRIVATE symengine)
# # 包含 Eigen 头文件
# target_include_directories(${PROJECT_NAME} PRIVATE ${EIGEN3_INCLUDE_DIR})

pretty_print_list("MIAN_SOURCE "                    MIAN_SOURCE)
pretty_print_list("MY_INCLUDE_DIRECTORIES List"     MY_INCLUDE_DIRECTORIES)
pretty_print_list("SOURCES List"                    SOURCES)
pretty_print_list("THIRD_INCLUDE_DIRECTORIES List"  THIRD_INCLUDE_DIRECTORIES)
print_colored_message("------------------------------" COLOR magenta)
# # -------------------------------- Unit test ------------------------------
option(ENABLE_TEST "Enable unit testing" ON)
if(ENABLE_TEST)
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Configuring Unit Tests..." COLOR yellow)
    include(cmake/setup_tests.cmake)
else()
    print_colored_message("Unit Tests are disabled." COLOR red)
endif()
# -------------------------------- Coverage ------------------------------
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
if(ENABLE_COVERAGE)
    # 给编译器添加覆盖率标志
    add_compile_options(--coverage)
    add_link_options(--coverage)
    # 引入独立出来的覆盖率配置
    include(cmake/setup_coverage.cmake)
endif()
# -------------------------------- CPack ------------------------------
option(ENABLE_CPACK "Enable CPack packaging" ON)
if(ENABLE_CPACK)
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Configuring CPack Packaging..." COLOR yellow)
    include(cmake/setup_cpack.cmake)
    print_colored_message("------------------------------" COLOR magenta)
else()
    print_colored_message("CPack Packaging is disabled." COLOR red)
endif()

