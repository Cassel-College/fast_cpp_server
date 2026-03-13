# =========================
# MAVSDK integration select
# =========================

add_library(my_mavsdk INTERFACE)
find_package(Threads REQUIRED)

set(IS_KYLIN FALSE)

if(EXISTS "/etc/os-release")
    file(STRINGS "/etc/os-release" OS_RELEASE_LINES)
    foreach(line IN LISTS OS_RELEASE_LINES)
        if(line MATCHES "^(ID|NAME|PRETTY_NAME)=" AND line MATCHES "[Kk]ylin")
            set(IS_KYLIN TRUE)
            break()
        endif()
    endforeach()
endif()

message(STATUS "IS_KYLIN = ${IS_KYLIN}")

if(IS_KYLIN)
    message(STATUS "Kylin system detected, prefer system MAVSDK")

    find_package(MAVSDK QUIET CONFIG)

    if(MAVSDK_FOUND)
        message(STATUS "Found MAVSDK by find_package(MAVSDK CONFIG)")
        target_link_libraries(my_mavsdk INTERFACE
            MAVSDK::mavsdk
            Threads::Threads
            dl
        )
    else()
        message(STATUS "find_package(MAVSDK) failed, fallback to manual search")

        find_path(MAVSDK_INCLUDE_DIR
            NAMES mavsdk/mavsdk.h
            PATHS
                /usr/include
                /usr/local/include
        )

        find_library(MAVSDK_LIBRARY
            NAMES mavsdk
            PATHS
                /usr/lib
                /usr/lib/aarch64-linux-gnu
                /lib
                /usr/local/lib
        )

        if(NOT MAVSDK_INCLUDE_DIR)
            message(FATAL_ERROR "System MAVSDK headers not found")
        endif()

        if(NOT MAVSDK_LIBRARY)
            message(FATAL_ERROR "System MAVSDK library not found")
        endif()

        message(STATUS "MAVSDK_INCLUDE_DIR = ${MAVSDK_INCLUDE_DIR}")
        message(STATUS "MAVSDK_LIBRARY = ${MAVSDK_LIBRARY}")

        target_include_directories(my_mavsdk INTERFACE
            ${MAVSDK_INCLUDE_DIR}
        )

        target_link_libraries(my_mavsdk INTERFACE
            ${MAVSDK_LIBRARY}
            Threads::Threads
            dl
        )
    endif()

else()
    message(STATUS "Non-Kylin system detected, using bundled MAVSDK")

    set(MAVSDK_DIST_DIR "${PROJECT_SOURCE_DIR}/build/mavsdk_dist")

    set(MAVSDK_LIBS
        mavsdk
        mav
        events
        jsoncpp
        tinyxml2
        curl
        ssl
        crypto
        lzma
        z
    )

    target_include_directories(my_mavsdk INTERFACE
        ${MAVSDK_DIST_DIR}/include
        ${MAVSDK_DIST_DIR}/include/mavsdk
        ${MAVSDK_DIST_DIR}/include/mavsdk/mavlink
        ${MAVSDK_DIST_DIR}/include/mavsdk/plugins
    )

    foreach(lib_name IN LISTS MAVSDK_LIBS)
        set(LIB_FILE "${MAVSDK_DIST_DIR}/lib/lib${lib_name}.a")

        if(NOT EXISTS "${LIB_FILE}")
            message(FATAL_ERROR "Missing: ${LIB_FILE}")
        endif()

        if(NOT TARGET ${lib_name})
            add_library(${lib_name} STATIC IMPORTED GLOBAL)
            set_target_properties(${lib_name} PROPERTIES
                IMPORTED_LOCATION "${LIB_FILE}"
            )
        endif()
    endforeach()

    target_link_libraries(my_mavsdk INTERFACE
        mavsdk
        mav
        events
        jsoncpp
        tinyxml2
        curl
        ldap
        lber
        ssl
        crypto
        lzma
        z
        nlohmann_json::nlohmann_json
        Threads::Threads
        dl
    )
endif()