cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

print_colored_message("------------------------------" COLOR magenta)
print_colored_message("Setting up SQLite (amalgamation)..." COLOR yellow)

set(SQLITE_DIR ${PROJECT_SOURCE_DIR}/external/sqlite)
set(SQLITE_C ${SQLITE_DIR}/sqlite3.c)
set(SQLITE_H ${SQLITE_DIR}/sqlite3.h)

if (NOT EXISTS ${SQLITE_C} OR NOT EXISTS ${SQLITE_H})
    print_colored_message("SQLite amalgamation not found at ${SQLITE_DIR}" COLOR red)
    message(FATAL_ERROR "Please download sqlite3.c/sqlite3.h into external/sqlite first.")
endif()

# 避免重复 add_library
if (TARGET sqlite3_static)
    print_colored_message("Target sqlite3_static already exists. ✅" COLOR green)
else()
    add_library(sqlite3_static STATIC ${SQLITE_C})
    target_include_directories(sqlite3_static PUBLIC ${SQLITE_DIR})

    # 推荐编译宏：确保线程安全（SQLite 编译时选项）
    target_compile_definitions(sqlite3_static PRIVATE
        SQLITE_THREADSAFE=1
    )

    # 对静态库设置 PIC，便于你未来如果把 my_db 编进 shared lib 也不出问题
    set_target_properties(sqlite3_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

    print_colored_message("SQLite configured: sqlite3_static ✅" COLOR green)
endif()

# 统一把 include 目录注入到 THIRD_INCLUDE_DIRECTORIES（若存在）
if (DEFINED THIRD_INCLUDE_DIRECTORIES)
    list(APPEND THIRD_INCLUDE_DIRECTORIES ${SQLITE_DIR})
    list(REMOVE_DUPLICATES THIRD_INCLUDE_DIRECTORIES)
endif()

print_colored_message("Setting up SQLite over." COLOR yellow)
print_colored_message("------------------------------" COLOR magenta)