# cmake/setup_dependencies.cmake
message(STATUS ">>> Loading Dependencies...")

# 初始化包含列表
message("初始化 THIRD_PARTY_INCLUDES 列表")
set(THIRD_PARTY_INCLUDES "")

include(cmake/deps/setup_oatpp.cmake)
include(cmake/deps/setup_protobuf.cmake)
include(cmake/deps/setup_spdlog.cmake)
include(cmake/deps/setup_yamlcpp.cmake)
include(cmake/deps/setup_json.cmake)
include(cmake/deps/setup_cpr.cmake)
include(cmake/deps/setup_mosquitto.cmake)
include(cmake/deps/setup_libzmq.cmake)
include(cmake/deps/setup_opencv.cmake)
include(cmake/deps/setup_eigen.cmake)
include(cmake/deps/setup_symengine.cmake)

# 将手机到的头文件路径暴露给父级
# set(THIRD_INCLUDE_DIRECTORIES ${THIRD_PARTY_INCLUDES} PARENT_SCOPE)