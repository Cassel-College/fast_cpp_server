cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_AUDIO)
    message("BUILD_MY_AUDIO is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_audio library..." COLOR yellow)

    # ========================================================================
    # 头文件路径
    # ========================================================================
    set(MY_AUDIO_INCLUDE_DIRECTORIES
        ${PROJECT_SOURCE_DIR}/src/util/my_audio
        ${PROJECT_SOURCE_DIR}/src/util/my_audio/NAudioCore
        ${PROJECT_SOURCE_DIR}/source/lib/NAudio/x86/include
    )

    # ========================================================================
    # 源文件收集（排除原始 Demo 文件）
    # ========================================================================
    file(GLOB_RECURSE MY_AUDIO_SOURCES
        ${PROJECT_SOURCE_DIR}/src/util/my_audio/*.cpp
    )

    pretty_print_list("MY_AUDIO_INCLUDE_DIRECTORIES List" MY_AUDIO_INCLUDE_DIRECTORIES)
    pretty_print_list("MY_AUDIO_SOURCES List" MY_AUDIO_SOURCES)

    # ========================================================================
    # 构建静态库
    # ========================================================================
    add_library(my_audio STATIC ${MY_AUDIO_SOURCES})
    target_include_directories(my_audio PUBLIC ${MY_AUDIO_INCLUDE_DIRECTORIES})

    # ========================================================================
    # 链接依赖
    # ========================================================================
    target_link_libraries(my_audio PUBLIC pthread)
    target_link_libraries(my_audio PUBLIC mylog)
    target_link_libraries(my_audio PUBLIC myconfig)
    target_link_libraries(my_audio PUBLIC my_tools)
    target_link_libraries(my_audio PUBLIC nlohmann_json::nlohmann_json)

    # NAudio 厂商 SDK 动态库目录（含 ffmpeg 等运行时依赖）
    set(NAUDIO_LIB_DIR ${PROJECT_SOURCE_DIR}/source/lib/NAudio/x86/lib)
    # 导出 NAUDIO_LIB_DIR 供其他 CMake 文件使用
    set(NAUDIO_LIB_DIR ${NAUDIO_LIB_DIR} PARENT_SCOPE)

    # 链接 NAudio SDK 及其运行时依赖
    # 使用 --no-as-needed 确保 ffmpeg 库成为可执行文件的直接依赖（NEEDED），
    # 因为 libNAudioServer.so 的 RUNPATH 是 "./"，无法在运行时找到同目录的 ffmpeg 库
    target_link_libraries(my_audio PUBLIC
        ${NAUDIO_LIB_DIR}/libNAudioServer.so
        -Wl,--no-as-needed
        ${NAUDIO_LIB_DIR}/libavcodec.so.58
        ${NAUDIO_LIB_DIR}/libavformat.so.58
        ${NAUDIO_LIB_DIR}/libavfilter.so.7
        ${NAUDIO_LIB_DIR}/libavutil.so.56
        ${NAUDIO_LIB_DIR}/libswresample.so.3
        ${NAUDIO_LIB_DIR}/libmp3lame.so.0
        ${NAUDIO_LIB_DIR}/libmsc.so
        -Wl,--as-needed
    )

    # 设置 RPATH，确保运行时能找到这些动态库
    set_target_properties(my_audio PROPERTIES
        BUILD_RPATH "${NAUDIO_LIB_DIR}"
        INSTALL_RPATH "${NAUDIO_LIB_DIR}"
    )

    print_colored_message("Building my_audio library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_AUDIO is OFF, skipping my_audio library build")
    return()
endif()
