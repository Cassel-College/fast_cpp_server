# cmake/setup_tests.cmake

print_colored_message("------------------------------" COLOR magenta)
print_colored_message("Configuring Unit Tests..." COLOR yellow)

include(cmake/deps/setup_gtest.cmake)

set(TEST_PROGRAM_NAME ${PROJECT_NAME}_Test)
file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")

pretty_print_list("TEST_SOURCES List" TEST_SOURCES)

add_executable(${TEST_PROGRAM_NAME} ${TEST_SOURCES})

# [关键修复]：直接链接第三方库 Target
# 现代 CMake 会自动把 opencv 和 symengine 的头文件路径传给测试程序
target_link_libraries(${TEST_PROGRAM_NAME} PRIVATE 
    gtest
    gtest_main
    pthread
    mylib                  # 链接你的主库
    mylog
    myconfig
    myproto
    my_arg_parser
    my_doctor
    my_mqtt
    my_soft_healthy
    my_system_healthy
    symengine              # 修复 symengine/expression.h 找不到
    opencv_core            # 修复 opencv2/opencv.hpp 找不到
    opencv_imgproc
    opencv_highgui
    opencv_calib3d
    opencv_core
    opencv_dnn
    opencv_features2d
    opencv_flann
    opencv_gapi
    opencv_highgui
    opencv_imgcodecs
    opencv_imgproc
    opencv_ml
    opencv_objdetect
    opencv_photo
    opencv_stitching
    opencv_video
    opencv_videoio
    oatpp::oatpp
    oatpp::oatpp-swagger
    cpr::cpr
    libzmq
    libmosquitto_static
)

# 包含内部路径
target_include_directories(${TEST_PROGRAM_NAME} PRIVATE 
    ${THIRD_INCLUDE_DIRECTORIES}
    ${ALL_INCLUDE_DIRS}    # 使用我们在 src/CMakeLists.txt 中搜集的路径
)

include(GoogleTest)
gtest_discover_tests(${TEST_PROGRAM_NAME} DISCOVERY_TIMEOUT 30)


option(ENABLE_TEST "Enable unit testing" ON)
# start test
if(ENABLE_TEST) 
    enable_testing()
    add_test(NAME ${PROJECT_NAME}UnitTest COMMAND ${TEST_PROGRAM_NAME})
endif()
print_colored_message("------------------------------" COLOR magenta)
