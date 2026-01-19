# --------------------- OpenCV 设置 ---------------------
# 指定需要构建的模块（可按需调整）,

set(BUILD_LIST calib3d,core,dnn,features2d,flann,gapi,highgui,imgcodecs,imgproc,ml,objdetect,photo,stitching,video,videoio)
# 是否构建为静态库
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs")
# opencv contrib 路径（可选）
set(OPENCV_EXTRA_MODULES_PATH ${CMAKE_SOURCE_DIR}/external/opencv_contrib/modules)
message(STATUS "OPENCV_EXTRA_MODULES_PATH: ${OPENCV_EXTRA_MODULES_PATH}")
# 不使用 protobuf
set(WITH_PROTOBUF OFF CACHE BOOL "Don't use protobuf in OpenCV")
set(INSTALL_CREATE_DISTRIBUTION OFF CACHE BOOL "" FORCE)
set(INSTALL_C_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INSTALL_PYTHON_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(OPENCV_GENERATE_PKGCONFIG OFF CACHE BOOL "" FORCE)
# 添加 OpenCV 子目录
add_subdirectory(external/opencv EXCLUDE_FROM_ALL)


# set(BUILD_LIST calib3d,core,dnn,features2d,flann,gapi,highgui,imgcodecs,imgproc,ml,objdetect,photo,stitching,video,videoio)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
# set(OPENCV_EXTRA_MODULES_PATH ${CMAKE_SOURCE_DIR}/external/opencv_contrib/modules)
# set(WITH_PROTOBUF OFF CACHE BOOL "Don't use protobuf in OpenCV")
# set(INSTALL_CREATE_DISTRIBUTION OFF CACHE BOOL "" FORCE)
# set(INSTALL_C_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(INSTALL_PYTHON_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(OPENCV_GENERATE_PKGCONFIG OFF CACHE BOOL "" FORCE)

# add_subdirectory(external/opencv EXCLUDE_FROM_ALL)

# list(APPEND THIRD_PARTY_INCLUDES
#     ${CMAKE_SOURCE_DIR}/external/opencv/include
#     ${CMAKE_SOURCE_DIR}/external/opencv/modules/core/include
#     ${CMAKE_SOURCE_DIR}/build # OpenCV 生成的头文件
# )