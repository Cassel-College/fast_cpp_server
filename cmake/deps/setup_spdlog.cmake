set(SPDLOG_BUILD_SHARED OFF)
add_subdirectory(external/spdlog EXCLUDE_FROM_ALL)

message(STATUS "添加 spdlog 包含路径到 THIRD_PARTY_INCLUDES")

list(APPEND THIRD_PARTY_INCLUDES ${CMAKE_SOURCE_DIR}/external/spdlog/include)