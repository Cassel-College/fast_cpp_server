# cmake/deps/setup_cpr.cmake

set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
set(CPR_ENABLE_SSL ON CACHE BOOL "" FORCE)

# 如果 external/cpr 存在
if(EXISTS "${CMAKE_SOURCE_DIR}/external/cpr")
    add_subdirectory(external/cpr EXCLUDE_FROM_ALL)
    # 显式添加路径到全局变量以防万一
    message(STATUS "添加 CPR 包含路径到 THIRD_PARTY_INCLUDES")
    message(STATUS "CPR include path: ${CMAKE_SOURCE_DIR}/external/cpr/include")
    list(APPEND THIRD_PARTY_INCLUDES ${CMAKE_SOURCE_DIR}/external/cpr/include)
else()
    message(FATAL_ERROR "CPR directory not found in external/cpr")
endif()