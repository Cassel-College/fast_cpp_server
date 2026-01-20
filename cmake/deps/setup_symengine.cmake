set(BUILD_SHARED_LIBS   OFF CACHE BOOL "Build static symengine" FORCE)
set(HAVE_SYMENGINE_GMP  OFF CACHE BOOL "Not use GMP")
set(SYMENGINE_USE_MPFR  OFF CACHE BOOL "")
set(SYMENGINE_USE_MPC   OFF CACHE BOOL "")
set(SYMENGINE_USE_FLINT OFF CACHE BOOL "")
set(BUILD_TESTS         OFF CACHE BOOL "")
set(BUILD_BENCHMARKS    OFF CACHE BOOL "")
set(BUILD_EXAMPLES      OFF CACHE BOOL "")

message(STATUS "添加 SymEngine 作为第三方库")
add_subdirectory(external/symengine EXCLUDE_FROM_ALL)



if (NOT EXISTS "${CMAKE_SOURCE_DIR}/external/symengine")
    message(FATAL_ERROR "SymEngine path not found: ${CMAKE_SOURCE_DIR}/external/symengine")
else()
    print_colored_message("<include> Found SymEngine include & build paths" COLOR green)
    
    # 1. 源码目录：解决基本头文件引用
    # 注意：如果代码里写的是 #include <symengine/basic.h>
    # 那么 INCLUDE 路径必须是其父目录，即 external/symengine
    list(APPEND THIRD_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/symengine")
    
    # 2. 构建目录：解决 symengine_config.h 找不到的问题
    # SymEngine 编译后，生成的配置文件通常在 build/external/symengine 下
    if(EXISTS "${CMAKE_BINARY_DIR}/external/symengine")
        list(APPEND THIRD_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/external/symengine")
    endif()
endif()

list(APPEND THIRD_PARTY_INCLUDES 
    ${CMAKE_SOURCE_DIR}/external/symengine
    ${CMAKE_SOURCE_DIR}/build/external/symengine
)