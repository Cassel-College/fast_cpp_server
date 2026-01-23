# cmake/setup_cpack.cmake

# --- 1. 定义控制开关 ---
option(ENABLE_INSTALL_MQTT "Whether to include MQTT broker and tools" ON)

# --- 2. 获取版本与 Git 信息 ---
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(GIT_COMMIT_ID "unknown")
    set(GIT_BRANCH "master")
endif()

string(TIMESTAMP CURRENT_TIME "%Y%m%d_%H%M%S")
set(FULL_VERSION "${GIT_BRANCH}-${GIT_COMMIT_ID}")

# --- 3. CPack 核心变量设置 (必须在 include(CPack) 之前) ---
set(CPACK_PACKAGE_NAME                  ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION               "${FULL_VERSION}")
set(CPACK_PACKAGE_FILE_NAME             "${CPACK_PACKAGE_NAME}-${FULL_VERSION}-${CURRENT_TIME}")
set(CPACK_PACKAGE_DIRECTORY             "${PROJECT_SOURCE_DIR}/releases")
set(CPACK_GENERATOR                     "TGZ")
set(CPACK_STRIP_FILES                   ON)
set(CPACK_SET_DESTDIR                   ON)
set(CPACK_INSTALL_PREFIX                "/")

# 核心：设置排除列表（使用更通用的正则表达式）
set(CPACK_INSTALL_IGNORED_FILES
    "/include/.*"
    "/sbin/.*"
    "/pkgconfig/.*"
    "\\\\.a$"
    "\\\\.pc$"
    "\\\\.h$"
    "\\\\.hpp$"
)

# 如果不集成 MQTT，额外排除
if(NOT ENABLE_INSTALL_MQTT)
    list(APPEND CPACK_INSTALL_IGNORED_FILES 
        "^/etc/.*" 
        "/mosquitto.*"
        "/libmosquitto.*"
    )
    print_colored_message("CPACK: MQTT DISABLED - Stripping MQTT files" COLOR yellow)
else()
    print_colored_message("CPACK: MQTT ENABLED - Including broker and tools" COLOR green)
endif()

# --- 4. 引入 CPack 模块 ---
include(CPack)

# --- 5. 定义安装路径变量 ---
set(CONFIG_DIRECTORIES      ${PROJECT_SOURCE_DIR}/config)
set(SERVICE_DIRECTORIES     ${PROJECT_SOURCE_DIR}/service)
set(SCRIPT_DIRECTORIES      ${PROJECT_SOURCE_DIR}/scripts)
set(SWAGGER_RES_DIR         ${CMAKE_SOURCE_DIR}/external/oatpp-swagger/res)
set(MQTT_ETC_DIR            ${PROJECT_SOURCE_DIR}/external/mosquitto/etc)

# --- 6. 安装规则 ---

# 6.1 主程序
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/config/ DESTINATION config)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/service/ DESTINATION service)

# 6.2 库文件 (只通过我们的规则安装 .so)
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/ 
        DESTINATION lib 
        FILES_MATCHING PATTERN "*.so*" 
        EXCLUDE PATTERN "pkgconfig" 
        EXCLUDE PATTERN "cmake")

# 6.3 Swagger
install(DIRECTORY ${SWAGGER_RES_DIR} DESTINATION swagger-res)

# 6.4 脚本 (使用 PROGRAMS 确保执行权限)
install(PROGRAMS ${SCRIPT_DIRECTORIES}/start.sh     DESTINATION .)
install(PROGRAMS ${SCRIPT_DIRECTORIES}/install.sh   DESTINATION .)
install(PROGRAMS ${SCRIPT_DIRECTORIES}/uninstall.sh DESTINATION .)

print_colored_message("------------------------------------------------" COLOR magenta)
# 7.5 条件安装 MQTT 相关组件
print_colored_message("MQTT 相关组件安装状态: ${ENABLE_INSTALL_MQTT}" COLOR yellow)
if(ENABLE_INSTALL_MQTT)
    print_colored_message("Configuring MQTT extra components for installation..." COLOR cyan)
    
    # 安装 Mosquitto 配置文件
    if(EXISTS ${MQTT_ETC_DIR})
        print_colored_message("存在 MQTT 配置目录: ${MQTT_ETC_DIR}，准备打包至 etc/ ..." COLOR green)
        install(DIRECTORY ${MQTT_ETC_DIR} DESTINATION etc)
    else()
        print_colored_message("警告: MQTT 配置目录不存在，跳过打包 etc/ ..." COLOR red)
    endif()

    # # 显式安装 libmosquitto 目标文件
    # if(TARGET libmosquitto)
    #     print_colored_message("准备打包 libmosquitto 库文件至 lib/ ..." COLOR green)
    #     install(FILES $<TARGET_FILE:libmosquitto> DESTINATION lib)
    # else()
    #     print_colored_message("警告: libmosquitto 目标不存在，跳过打包 lib/ ..." COLOR red)
    # endif()
    # 显式安装 mosquitto 库文件（兼容 static/shared，避免安装不存在的文件导致 CPack 失败）
    set(_installed_mosquitto_lib FALSE)

    # 1) 优先安装静态库（你当前实际生成了 libmosquitto_static.a）
    if(TARGET mosquitto_static)
        print_colored_message("准备打包 mosquitto_static 库文件至 lib/ ..." COLOR green)
        install(FILES $<TARGET_FILE:mosquitto_static> DESTINATION lib)
        set(_installed_mosquitto_lib TRUE)
    elseif(TARGET libmosquitto_static)
        print_colored_message("准备打包 libmosquitto_static 库文件至 lib/ ..." COLOR green)
        install(FILES $<TARGET_FILE:libmosquitto_static> DESTINATION lib)
        set(_installed_mosquitto_lib TRUE)
    endif()

    # 2) 如果存在共享库 target，再安装共享库
    # 注意：不要假设它一定存在；而且不要因为它存在就装一个不存在的输出文件
    if(NOT _installed_mosquitto_lib)
        if(TARGET mosquitto) # 有些版本 target 叫 mosquitto
            print_colored_message("准备打包 mosquitto(shared) 库文件至 lib/ ..." COLOR green)
            install(FILES $<TARGET_FILE:mosquitto> DESTINATION lib)
            set(_installed_mosquitto_lib TRUE)
        elseif(TARGET libmosquitto) # 你原来用的
            print_colored_message("准备打包 libmosquitto(shared) 库文件至 lib/ ..." COLOR green)
            install(FILES $<TARGET_FILE:libmosquitto> DESTINATION lib)
            set(_installed_mosquitto_lib TRUE)
        endif()
    endif()

    # 3) 最后兜底：如果 target 名不匹配，但文件确实生成了，就用 glob 安装（只在 configure 阶段存在时才会执行）
    if(NOT _installed_mosquitto_lib)
        file(GLOB _mosq_candidates
            "${CMAKE_BINARY_DIR}/lib/libmosquitto*.so*"
            "${CMAKE_BINARY_DIR}/lib/libmosquitto*.a"
        )
        if(_mosq_candidates)
            print_colored_message("准备打包 mosquitto 库文件(兜底 glob) 至 lib/ ..." COLOR green)
            install(FILES ${_mosq_candidates} DESTINATION lib)
            set(_installed_mosquitto_lib TRUE)
        else()
            print_colored_message("警告: 未找到 mosquitto 库文件产物，跳过打包 lib/ ..." COLOR red)
        endif()
    endif()
    
    # 安装 mosquitto 可执行文件
    find_program(MOSQUITTO_EXECUTABLE mosquitto)
    if(MOSQUITTO_EXECUTABLE)
        print_colored_message("找到 mosquitto 可执行文件: ${MOSQUITTO_EXECUTABLE}，准备打包至 bin/ ..." COLOR green)
        install(PROGRAMS ${MOSQUITTO_EXECUTABLE} DESTINATION bin)
    else()
        print_colored_message("警告: 未找到 mosquitto 可执行文件，跳过打包 bin/ ..." COLOR red)
    endif()

    print_colored_message("CPACK: MQTT components will be located in bin/ and etc/" COLOR cyan)
endif()

# 7.6 其他依赖
if(TARGET libzmq)
    install(FILES $<TARGET_FILE:libzmq> DESTINATION lib)
endif()