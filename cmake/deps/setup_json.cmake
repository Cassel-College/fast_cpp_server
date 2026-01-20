

message(STATUS "添加 nlohmann/json 库到 THIRD_PARTY_INCLUDES")

if (EXISTS "${PROJECT_SOURCE_DIR}/external/json/include")
    list(APPEND THIRD_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/external/json/include")
    print_colored_message("<inlcude> Found nlohmann/json include" COLOR green)
else()
    print_colored_message("nlohmann/json include not found at ${PROJECT_SOURCE_DIR}/external/json/include" COLOR yellow)
endif()

add_subdirectory(external/json)

