# =========================
# ViewLink SDK integration
# =========================
# 品凌云台使用 ViewLink SDK（深圳景鹏科技 / Viewpro）
# 预编译静态库通过 build.sh 复制到 build/viewlink_dist/

add_library(my_viewlink INTERFACE)
find_package(Threads REQUIRED)

set(VIEWLINK_DIST_DIR "${PROJECT_SOURCE_DIR}/build/viewlink_dist")

# --- 检查头文件 ---
if(NOT EXISTS "${VIEWLINK_DIST_DIR}/include/ViewLink.h")
    message(FATAL_ERROR "ViewLink header not found: ${VIEWLINK_DIST_DIR}/include/ViewLink.h\n"
                        "请先运行 build.sh 以复制 ViewLink SDK 到 build/viewlink_dist/")
endif()

# --- 检查静态库 ---
set(VIEWLINK_LIB_FILE "${VIEWLINK_DIST_DIR}/lib/libViewLink.a")
if(NOT EXISTS "${VIEWLINK_LIB_FILE}")
    message(FATAL_ERROR "ViewLink library not found: ${VIEWLINK_LIB_FILE}\n"
                        "请先运行 build.sh 以复制 ViewLink SDK 到 build/viewlink_dist/")
endif()

# --- 注册 IMPORTED 静态库 ---
if(NOT TARGET ViewLink)
    add_library(ViewLink STATIC IMPORTED GLOBAL)
    set_target_properties(ViewLink PROPERTIES
        IMPORTED_LOCATION "${VIEWLINK_LIB_FILE}"
    )
endif()

# --- 对外暴露 INTERFACE 目标 ---
target_include_directories(my_viewlink INTERFACE
    ${VIEWLINK_DIST_DIR}/include
)

target_link_libraries(my_viewlink INTERFACE
    ViewLink
    Threads::Threads
    dl
)

message(STATUS "ViewLink SDK configured: ${VIEWLINK_DIST_DIR}")