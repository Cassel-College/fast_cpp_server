cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

# 项目信息
project(fast_cpp_server)
# project(consys_uuv_edge)

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

# function(pretty_print_list title list_var)
#     # 定义颜色
#     string(ASCII 27 Esc)
#     set(ColorGreen "${Esc}[32m")
#     set(ColorCyan  "${Esc}[36m")
#     set(ColorReset "${Esc}[0m")

#     # 打印标题
#     message(STATUS "${ColorCyan}${title}:${ColorReset}")

#     # 获取传入列表变量的内容
#     set(list_content "${${list_var}}")

#     # 找出最长项长度
#     set(max_len 0)
#     foreach(item ${list_content})
#         string(LENGTH "${item}" len)
#         if(len GREATER max_len)
#             set(max_len ${len})
#         endif()
#     endforeach()

#     # 打印每一项（对齐）
#     foreach(item ${list_content})
#         string(LENGTH "${item}" len)
#         math(EXPR padding "${max_len} - ${len}")
#         string(REPEAT " " ${padding} spaces)
#         message(STATUS "${ColorGreen}item:${ColorReset} ${item}${spaces}")
#     endforeach()
# endfunction()

# function(print_colored_message MESSAGE_STRING)
#     set(options BOLD)
#     set(oneValueArgs COLOR)
#     cmake_parse_arguments(PARSE_ARGV 1 ARG "${options}" "${oneValueArgs}" "")

#     # ANSI escape codes
#     string(ASCII 27 Esc)

#     set(color_reset "${Esc}[0m")
#     set(color_bold "${Esc}[1m")

#     # Define color codes
#     set(color_red "${Esc}[31m")
#     set(color_green "${Esc}[32m")
#     set(color_yellow "${Esc}[33m")
#     set(color_blue "${Esc}[34m")
#     set(color_magenta "${Esc}[35m")
#     set(color_cyan "${Esc}[36m")
#     set(color_white "${Esc}[37m")

#     # Get selected color
#     set(color_code "")
#     if(ARG_COLOR STREQUAL "red")
#         set(color_code "${color_red}")
#     elseif(ARG_COLOR STREQUAL "green")
#         set(color_code "${color_green}")
#     elseif(ARG_COLOR STREQUAL "yellow")
#         set(color_code "${color_yellow}")
#     elseif(ARG_COLOR STREQUAL "blue")
#         set(color_code "${color_blue}")
#     elseif(ARG_COLOR STREQUAL "magenta")
#         set(color_code "${color_magenta}")
#     elseif(ARG_COLOR STREQUAL "cyan")
#         set(color_code "${color_cyan}")
#     elseif(ARG_COLOR STREQUAL "white")
#         set(color_code "${color_white}")
#     endif()

#     # Add bold if requested
#     if(ARG_BOLD)
#         set(color_code "${color_bold}${color_code}")
#     endif()

#     # Print the colored message
#     message("${color_code}${MESSAGE_STRING}${color_reset}")
# endfunction()

message(STATUS ">>> 正在加载第三方依赖...")
# include(cmake/deps/setup_oatpp.cmake)
include(cmake/setup_dependencies.cmake)


# # 添加protobuf库
# # Protobuf: 明确禁止安装
# set(protobuf_INSTALL OFF CACHE BOOL "Disable protobuf install" FORCE)
# set(protobuf_BUILD_TESTS OFF CACHE BOOL "Disable protobuf tests")
# set(protobuf_USE_EXTERNAL_GTEST OFF CACHE BOOL "Disable external GTest usage")
# add_subdirectory(external/protobuf EXCLUDE_FROM_ALL)
# # 假设你生成后的 protoc 可执行文件在 build/bin 目录中
# set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protoc>)
# # set(Protobuf_PROTOC_EXECUTABLE ${PROJECT_SOURCE_DIR}/build/bin/protoc)
# message(STATUS "Protobuf protoc executable: ${Protobuf_PROTOC_EXECUTABLE}")
# # 添加easyloggingpp库
# set(build_static_lib ON)
# add_subdirectory(external/easyloggingpp)
# # 设置spdlog为静态库
# set(SPDLOG_BUILD_SHARED OFF)
# # 添加spdlog库
# add_subdirectory(external/spdlog EXCLUDE_FROM_ALL)
# # 添加 yaml-cpp 库
# add_subdirectory(external/yaml-cpp EXCLUDE_FROM_ALL)
# 添加日志模块
set(BUILD_MY_LOG ON CACHE BOOL "Build mylog library")
add_subdirectory(src/util/my_log)
# # 添加nlohmann/json库
# add_subdirectory(external/json)
# 添加 myconfig 库
set(BUILD_MY_CONFIG ON CACHE BOOL "Build my_config library")
add_subdirectory(src/util/my_config)
# # 添加cpr库
# # 强制使用系统 curl
# set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
# # CPR 自身
# set(CPR_ENABLE_SSL ON CACHE BOOL "" FORCE)
# add_subdirectory(external/cpr EXCLUDE_FROM_ALL)
# # 添加mosquitto库
# set(DOCUMENTATION OFF CACHE BOOL "Disable documentation generation")
# add_subdirectory(external/mosquitto)
# # 添加libzmq库
# set(WITH_TLS OFF)
# add_subdirectory(external/libzmq EXCLUDE_FROM_ALL)

# # --------------------- OpenCV 设置 ---------------------
# # 指定需要构建的模块（可按需调整）,

# set(BUILD_LIST calib3d,core,dnn,features2d,flann,gapi,highgui,imgcodecs,imgproc,ml,objdetect,photo,stitching,video,videoio)
# # 是否构建为静态库
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs")
# # opencv contrib 路径（可选）
# set(OPENCV_EXTRA_MODULES_PATH ${CMAKE_SOURCE_DIR}/external/opencv_contrib/modules)
# set(WITH_PROTOBUF OFF CACHE BOOL "Don't use protobuf in OpenCV")
# set(INSTALL_CREATE_DISTRIBUTION OFF CACHE BOOL "" FORCE)
# set(INSTALL_C_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(INSTALL_PYTHON_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(OPENCV_GENERATE_PKGCONFIG OFF CACHE BOOL "" FORCE)
# # 添加 OpenCV 子目录
# add_subdirectory(external/opencv EXCLUDE_FROM_ALL)

# --------------------- Eigen 设置 ---------------------
# Eigen 是头文件库，不需要编译，只需添加路径
set(EIGEN3_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/eigen)
include_directories(${EIGEN3_INCLUDE_DIR})

# --------------------- SymEngine 设置 ---------------------
# 启用静态构建（可选）
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static symengine")
set(HAVE_SYMENGINE_GMP OFF CACHE BOOL "Not use GMP")
set(SYMENGINE_USE_MPFR OFF CACHE BOOL "")
set(SYMENGINE_USE_MPC OFF CACHE BOOL "")
set(SYMENGINE_USE_FLINT OFF CACHE BOOL "")
# 关闭测试与示例
set(BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_BENCHMARKS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(external/symengine EXCLUDE_FROM_ALL)

# 添加 myproto 库
set(BUILD_MY_PROTO ON CACHE BOOL "Build myproto library")
add_subdirectory(src/protobuf)
# 添加 my_arg_parser 库
set(BUILD_MY_ARG_PARSER ON CACHE BOOL "Build my_arg_parser library")
add_subdirectory(src/util/my_arg_parser)
# 添加 my_soft_healthy 库
set(BUILD_MY_SOFT_HEALTHY ON CACHE BOOL "Build my_soft_healthy library")
add_subdirectory(src/util/my_soft_healthy)
# 添加 my_system_healthy 库
set(BUILD_MY_SYSTEM_HEALTHY ON CACHE BOOL "Build my_system_healthy library")
add_subdirectory(src/util/my_system_healthy)
# 添加 my_heartbeat 库
set(BUILD_MY_HEARTBEAT ON CACHE BOOL "Build my_heartbeat library")
add_subdirectory(src/util/my_heartbeat)
# 添加 my_cache 库
set(BUILD_MY_CACHE ON CACHE BOOL "Build my_cache library")
add_subdirectory(src/util/my_cache)
# 添加 my_db 库
set(BUILD_MY_DB ON CACHE BOOL "Build my_db library")
add_subdirectory(src/util/my_db)
# 添加 my_api 库
set(BUILD_MY_API ON CACHE BOOL "Build my_api library")
add_subdirectory(src/util/my_api)
# 添加 my_edge 库
set(BUILD_MY_EDGE ON CACHE BOOL "Build my_edge library")
add_subdirectory(src/util/my_edge)
# 添加 my_data 库
set(BUILD_MY_DATA ON CACHE BOOL "Build my_data library")
add_subdirectory(src/util/my_data)
# 添加 my_control 库
set(BUILD_MY_CONTROL ON CACHE BOOL "Build my_control library")
add_subdirectory(src/util/my_control)
# 添加 my_comm 库
set(BUILD_MY_COMM ON CACHE BOOL "Build my_comm library")
add_subdirectory(src/util/my_comm)
# 添加 my_ai_interface 库
set(BUILD_MY_AI_INTERFACE ON CACHE BOOL "Build my_ai_interface library")
add_subdirectory(src/util/my_ai_interface)
# 添加 my_network 库
set(BUILD_MY_NETWORK ON CACHE BOOL "Build my_network library")
add_subdirectory(src/util/my_network)
# 添加 mylib 库
set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage for mylib")
add_subdirectory(src)

set(THIRD_INCLUDE_DIRECTORIES 
    ${PROJECT_SOURCE_DIR}/external/spdlog/include
    ${PROJECT_SOURCE_DIR}/external/libzmq/include
    # ${PROJECT_SOURCE_DIR}/external/easyloggingpp/src
    ${PROJECT_SOURCE_DIR}/external/protobuf/src
    ${PROJECT_SOURCE_DIR}/external/eigen                            # Eigen 是头文件-only
    ${PROJECT_SOURCE_DIR}/external/symengine
    ${PROJECT_SOURCE_DIR}/external/opencv/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/calib3d/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/core/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/dnn/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/features2d/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/flann/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/gapi/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/highgui/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/imgcodecs/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/imgproc/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/ml/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/objdetect/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/photo/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/stitching/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/video/include
    ${PROJECT_SOURCE_DIR}/external/opencv/modules/videoio/include
    ${PROJECT_SOURCE_DIR}/build                                     # OpenCV 编译后的头文件路径
    ${PROJECT_SOURCE_DIR}/build/external/symengine                  # SymEngine 编译后的头文件路径
    ${PROJECT_SOURCE_DIR}/external/oatpp/src             # oatpp 核心头文件
    ${PROJECT_SOURCE_DIR}/external/oatpp-swagger/src     # oatpp-swagger 头文件
)

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

# # 链接spdlog静态库，使用 PRIVATE 关键字
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
target_link_libraries(${PROJECT_NAME} PRIVATE my_cache)
target_link_libraries(${PROJECT_NAME} PRIVATE mylib)
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

# 链接 Engine（Eigen 是头文件库，不需要 link）
# 链接 SymEngine
target_link_libraries(${PROJECT_NAME} PRIVATE symengine)
# # 包含 Eigen 头文件
# target_include_directories(${PROJECT_NAME} PRIVATE ${EIGEN3_INCLUDE_DIR})

add_dependencies(mylog spdlog::spdlog)
add_dependencies(mylib mylog)
add_dependencies(mylib my_heartbeat)
add_dependencies(mylib my_edge)
add_dependencies(mylib my_db)
add_dependencies(mylib my_cache)
add_dependencies(mylib myconfig)
add_dependencies(mylib my_comm)
add_dependencies(mylib my_control)
add_dependencies(mylib my_network)
add_dependencies(mylib my_ai_interface)
add_dependencies(mylib my_data)
add_dependencies(mylib myproto)
add_dependencies(mylib my_soft_healthy)
add_dependencies(mylib libzmq)
add_dependencies(mylib yaml-cpp)
add_dependencies(mylib cpr::cpr)
add_dependencies(mylib nlohmann_json::nlohmann_json)
add_dependencies(mylib symengine)
# add_dependencies(mylib oatpp::oatpp)
# add_dependencies(mylib oatpp::oatpp-swagger)

# # 依赖 opencv lib
# add_dependencies(mylib opencv_calib3d)
# add_dependencies(mylib opencv_core)
# add_dependencies(mylib opencv_dnn)
# add_dependencies(mylib opencv_features2d)
# add_dependencies(mylib opencv_flann)
# add_dependencies(mylib opencv_gapi)
# add_dependencies(mylib opencv_highgui)
# add_dependencies(mylib opencv_imgcodecs)
# add_dependencies(mylib opencv_imgproc)
# add_dependencies(mylib opencv_ml)
# add_dependencies(mylib opencv_objdetect)
# add_dependencies(mylib opencv_photo)
# add_dependencies(mylib opencv_stitching)
# add_dependencies(mylib opencv_video)
# add_dependencies(mylib opencv_videoio)


pretty_print_list("MIAN_SOURCE " MIAN_SOURCE)
pretty_print_list("MY_INCLUDE_DIRECTORIES List" MY_INCLUDE_DIRECTORIES)
pretty_print_list("SOURCES List" SOURCES)
pretty_print_list("THIRD_INCLUDE_DIRECTORIES List" THIRD_INCLUDE_DIRECTORIES)
print_colored_message("------------------------------" COLOR magenta)

# -------------------------------- Unit test ------------------------------
print_colored_message("------------------------------" COLOR magenta)
print_colored_message("Building Unit Test soft ..." COLOR yellow)
set(TEST_PROGRAM_NAME ${PROJECT_NAME}_Test)
# find all test source files
file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")
# 添加google test库
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(external/googletest)
pretty_print_list("TEST_SOURCES List" TEST_SOURCES)
# 添加测试
add_executable(${TEST_PROGRAM_NAME} ${TEST_SOURCES})
target_include_directories(
    ${TEST_PROGRAM_NAME} PRIVATE ${THIRD_INCLUDE_DIRECTORIES}
)

target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE gtest gtest_main)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE pthread)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE mylog)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE myconfig)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE myproto)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE my_arg_parser)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE mylib)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE my_soft_healthy)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE my_system_healthy)
# 链接 OpenCV 库
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_calib3d)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_core)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_dnn)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_features2d)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_flann)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_gapi)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_highgui)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_imgcodecs)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_imgproc)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_ml)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_objdetect)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_photo)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_stitching)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_video)
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE opencv_videoio)
# target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE oatpp::oatpp)
# target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE oatpp::oatpp-swagger)

    
# 链接 SymEngine（Eigen 是头文件库，不需要 link）
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE symengine)
# 包含 Eigen 头文件
target_include_directories(${TEST_PROGRAM_NAME} PRIVATE ${EIGEN3_INCLUDE_DIR})


include(GoogleTest)
gtest_discover_tests(${TEST_PROGRAM_NAME}
    TEST_PREFIX "${TEST_PROGRAM_NAME}."
    DISCOVERY_TIMEOUT 30
)

option(ENABLE_TEST "Enable unit testing" ON)
# start test
if(ENABLE_TEST) 
    enable_testing()
    add_test(NAME ${PROJECT_NAME}UnitTest COMMAND ${TEST_PROGRAM_NAME})
endif()
print_colored_message("------------------------------" COLOR magenta)

# -------------------------------- Coverage ------------------------------
# custom target for coverage
add_custom_target(coverage
    COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage_report.html --exclude '${CMAKE_SOURCE_DIR}/external/.*'
    DEPENDS ${PROJECT_NAME}
)

# -------------------------------- CPack ------------------------------
# --- 1. 获取 Git Commit ID ---
find_package(Git QUIET)
if(GIT_FOUND)
    # 获取短 Hash (前 7 位)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # 获取当前分支名称 (可选)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_COMMIT_ID "unknown")
    set(GIT_BRANCH "master")
endif()
# --- 2. 组合版本号 ---
# 获取当前时间
string(TIMESTAMP CURRENT_TIME "%Y%m%d_%H%M%S")
# 格式：主版本.次版本.修订号-GitID
set(MAIN_VERSION "1.0.0")
# set(FULL_VERSION "${GIT_BRANCH}-${GIT_COMMIT_ID}-${MAIN_VERSION}")
set(FULL_VERSION "${GIT_BRANCH}-${GIT_COMMIT_ID}")
# --- 3. 设置 CPack 变量 ---
# 设置安装包的基本信息
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION "${FULL_VERSION}")
set(CPACK_DEBIAN_PACKAGE_VERSION "${FULL_VERSION}") # 如果打 DEB 包
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${FULL_VERSION}-${CURRENT_TIME}")
# 动态设置打包路径
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/releases")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} - A high-performance C++ server")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_CONTACT "your_email@example.com")
set(CPACK_SET_DESTDIR ON)
set(CPACK_INSTALL_PREFIX "/")   # 将安装前缀设为根，配合相对路径使用
set(CPACK_STRIP_FILES ON)       # 安装包中去除符号表，减小体积

# -------------------------------- CPack 强力清理 ------------------------------

# 使用正则排除不需要的文件/目录
# 注意：Regex 匹配的是安装后的相对路径
set(CPACK_INSTALL_IGNORED_FILES
    # 1. 排除所有头文件目录 (你的程序运行不需要 include)
    "/include/"
    
    # 2. 排除所有静态库 (.a) -> 你的 bin 已经静态链接了它们
    "\\\\.a$"
    
    # 3. 排除 cmake 和 pkgconfig 配置目录
    "/lib/cmake/"
    "/lib/pkgconfig/"
    "/share/pkgconfig/"
    
    # 4. 排除文档和无关数据
    "/share/doc/"
    "/share/man/"
    "/share/opencv4/"
    "/share/zmq/"
    "/share/licenses/"
    
    # 5. 排除不需要的 OpenCV/Mosquitto/ZMQ 工具 (保留你自己的 bin)
    "/bin/opencv_"
    "/bin/mosquitto_"
    "/bin/protoc"
    "/bin/inproc_"  # 如果这些是你不需要的 ZMQ 测试工具
    "/bin/local_"
    "/bin/remote_"
    "/bin/proxy_"
    "/setup_vars_opencv4.sh"
    
    # 6. 排除 /usr/local 这种奇怪的绝对路径残留
    "^/usr/"
)

# -----------------------------------------------------------------------------
# 启用 CPack 模块
include(CPack)

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/")
# 设置安装包的生成格式（如 .tar.gz、.deb、.rpm 等）
set(CPACK_GENERATOR "TGZ") # 生成 .tar.gz 包； # 其他可选格式：ZIP、DEB、RPM 等
# 设置安装路径
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
set(SCRIPT_DIRECTORIES ${PROJECT_SOURCE_DIR}/scripts)
set(SWAGGER_RES_INSTALL_DIR "/usr/share/${PROJECT_NAME}/swagger-res")
set(SWAGGER_RES_DIR         ${CMAKE_SOURCE_DIR}/external/oatpp-swagger/res)
# --- 编译时定义宏 ---
# 使用 CMAKE_INSTALL_PREFIX 配合相对路径，确保在开发阶段和安装后都能找到
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # 开发阶段，指向源码路径
    add_definitions(-DOATPP_SWAGGER_RES_PATH="${CMAKE_SOURCE_DIR}/external/oatpp-swagger/res")
else()
    # 发布阶段，指向安装后的相对路径 (相对于安装根目录)
    # 注意：实际运行时可能需要根据可执行文件位置计算绝对路径
    add_definitions(-DOATPP_SWAGGER_RES_PATH="${CMAKE_INSTALL_PREFIX}/${SWAGGER_RES_INSTALL_DIR}")
endif()

# 1. 安装主程序
install(TARGETS     ${PROJECT_NAME}      RUNTIME        DESTINATION bin)
# 2. (可选) 安装单元测试程序 - 生产环境建议注释掉
install(TARGETS     ${TEST_PROGRAM_NAME} RUNTIME        DESTINATION bin)
# 3. 安装配置目录
install(DIRECTORY   ${PROJECT_SOURCE_DIR}/config/       DESTINATION config)
install(DIRECTORY   ${PROJECT_SOURCE_DIR}/service/      DESTINATION service)
# 4. 安装 Swagger 资源文件
install(DIRECTORY   ${SWAGGER_RES_DIR}                  DESTINATION swagger-res)
# 5. 安装脚本
install(FILES       ${SCRIPT_DIRECTORIES}/start.sh      DESTINATION .)
install(FILES       ${SCRIPT_DIRECTORIES}/install.sh    DESTINATION .)
install(FILES       ${SCRIPT_DIRECTORIES}/uninstall.sh  DESTINATION .)
# 6. [关键] 仅安装运行时必须的动态库 (.so)
# install(DIRECTORY   ${CMAKE_BINARY_DIR}/lib/ DESTINATION lib)
if(TARGET libzmq)
    install(FILES $<TARGET_FILE:libzmq> DESTINATION lib)
endif()

if(TARGET libmosquitto)
    install(FILES $<TARGET_FILE:libmosquitto> DESTINATION lib)
endif()