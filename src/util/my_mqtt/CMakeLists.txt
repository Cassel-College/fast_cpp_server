cmake_minimum_required(VERSION 3.10)

if (BUILD_MY_MQTT)
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_mqtt library..." COLOR yellow)
    
    # 修正路径：确保指向包含 fast_mqtt_client 等文件夹的父目录
    set(MY_MQTT_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_mqtt)

    # 使用 APPEND 避免变量被覆盖
    file(GLOB_RECURSE MY_MQTT_SOURCES 
        "${PROJECT_SOURCE_DIR}/src/util/my_mqtt/fast_mqtt_client/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/util/my_mqtt/fast_mqtt_server/*.cpp"
    )

    pretty_print_list("MY_MQTT_SOURCES List" MY_MQTT_SOURCES)
    add_library(my_mqtt STATIC ${MY_MQTT_SOURCES})
    
    # 把本模块 include 和第三方 include 一并加入（THIRD_INCLUDE_DIRECTORIES 由 setup_mosquitto.cmake 填充）
    # target_include_directories(my_mqtt PUBLIC ${MY_MQTT_INCLUDE_DIRECTORIES} ${THIRD_INCLUDE_DIRECTORIES})
    target_include_directories(my_mqtt PUBLIC ${MY_MQTT_INCLUDE_DIRECTORIES})

    # 优先使用工程内的 libmosquitto_static target（保证不依赖系统开发包）
    if (TARGET libmosquitto_static)
        message(STATUS "Linking my_mqtt with project libmosquitto_static")
        target_link_libraries(my_mqtt PUBLIC 
            libmosquitto_static
            pthread
            mylog
            myconfig
        )
        # 确保包含 external/mosquitto/include（冗余但稳妥）
        target_include_directories(my_mqtt PUBLIC ${CMAKE_SOURCE_DIR}/external/mosquitto/include)
    else()
        message(FATAL_ERROR "libmosquitto_static not found in project. Ensure setup_mosquitto.cmake is processed before this CMakeLists and libmosquitto_static is built.")
    endif()

    # 为静态库开启 PIC，提高可移植性
    set_target_properties(my_mqtt PROPERTIES POSITION_INDEPENDENT_CODE ON)

    print_colored_message("Building my_mqtt library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
endif()