# cmake/deps/setup_cpr.cmake

set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
set(CPR_ENABLE_SSL ON CACHE BOOL "" FORCE)

# 如果 external/cpr 存在
if(EXISTS "${CMAKE_SOURCE_DIR}/external/cpr")
    add_subdirectory(external/cpr EXCLUDE_FROM_ALL)
    # 显式添加路径到全局变量以防万一
    message(STATUS "添加 CPR 包含路径到 THIRD_PARTY_INCLUDES")
    print_colored_message("设置 CPR include path: ${PROJECT_SOURCE_DIR}/external/cpr/include" COLOR green)
    if(EXISTS "${PROJECT_SOURCE_DIR}/external/spdlog/include")
        list(APPEND THIRD_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/external/spdlog/include")
        print_colored_message("<include> Found spdlog include" COLOR green)
    else()
        print_colored_message("spdlog include not found at ${PROJECT_SOURCE_DIR}/external/spdlog/include" COLOR yellow)
    endif()
else()
    message(FATAL_ERROR "CPR directory not found in external/cpr")
endif()