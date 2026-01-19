# cmake/setup_cpack.cmake

# --- 版本控制 ---
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE GIT_COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(GIT_COMMIT_ID "unknown")
    set(GIT_BRANCH "master")
endif()

string(TIMESTAMP CURRENT_TIME "%Y%m%d_%H%M%S")
set(FULL_VERSION "${GIT_BRANCH}-${GIT_COMMIT_ID}")

# --- CPack 基础设置 ---
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION "${FULL_VERSION}")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/releases")
set(CPACK_GENERATOR "TGZ") # 可改为 DEB
set(CPACK_SET_DESTDIR ON)
set(CPACK_INSTALL_PREFIX "/")
set(CPACK_STRIP_FILES ON)

# --- 忽略文件 ---
set(CPACK_INSTALL_IGNORED_FILES
    "/include/"
    "\\\\.a$"
    "/lib/cmake/"
    "/lib/pkgconfig/"
    "/share/doc/"
    "/share/man/"
    "/bin/opencv_"
    "/bin/mosquitto_"
    "/bin/protoc"
    "^/usr/"
)

# --- 安装规则 ---

# 1. 主程序与脚本
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/config/ DESTINATION config)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/service/ DESTINATION service)
install(FILES ${PROJECT_SOURCE_DIR}/scripts/start.sh DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/scripts/install.sh DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/scripts/uninstall.sh DESTINATION .)

# 2. Swagger 资源
set(SWAGGER_RES_DIR ${CMAKE_SOURCE_DIR}/external/oatpp-swagger/res)
install(DIRECTORY ${SWAGGER_RES_DIR}/ DESTINATION swagger-res)

# 3. 动态库 (关键修复：使用 Targets 安装)
if(TARGET libzmq)
    install(TARGETS libzmq LIBRARY DESTINATION lib)
endif()

if(TARGET mosquitto) 
    # 注意：mosquitto 的 target 名可能叫 mosquitto 或 libmosquitto
    install(TARGETS mosquitto LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
elseif(TARGET libmosquitto)
    install(TARGETS libmosquitto LIBRARY DESTINATION lib)
endif()

include(CPack)