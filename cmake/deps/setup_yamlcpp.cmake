

message(STATUS "添加 yaml-cpp 包含路径到 THIRD_PARTY_INCLUDES")

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/external/yaml-cpp/include")
    message(FATAL_ERROR "yaml-cpp include path not found: ${CMAKE_SOURCE_DIR}/external/yaml-cpp/include")
else()
    print_colored_message("<include> Found yaml-cpp include" COLOR green)
    list(APPEND THIRD_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/external/yaml-cpp/include)
endif()

add_subdirectory(external/yaml-cpp EXCLUDE_FROM_ALL)