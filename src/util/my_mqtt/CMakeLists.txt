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
        "${PROJECT_SOURCE_DIR}/src/util/my_mqtt/fast_mqtt_singleton_client/*.cpp"
    )

    # 查找 Mosquitto 库路径，避开名为 "mosquitto" 的 Executable Target
    # find_library(MOSQUITTO_LIBRARY NAMES mosquitto)

    add_library(my_mqtt STATIC ${MY_MQTT_SOURCES})
    
    target_include_directories(my_mqtt PUBLIC ${MY_MQTT_INCLUDE_DIRECTORIES})
    
    target_link_libraries(my_mqtt PUBLIC 
        pthread 
        mylog 
        myconfig 
        libmosquitto_static
    )

    print_colored_message("Building my_mqtt library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
endif()