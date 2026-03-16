cmake_minimum_required(VERSION 3.10)

message("CMAKE_VERSION: ${CMAKE_VERSION}")

print_colored_message("------------------------------" COLOR magenta)
print_colored_message("Setting up serial (manual sources)..." COLOR yellow)

set(SERIAL_ROOT ${PROJECT_SOURCE_DIR}/external/serial)
set(SERIAL_INCLUDE_DIR ${SERIAL_ROOT}/include)
set(SERIAL_SOURCE_DIR ${SERIAL_ROOT}/src)

set(SERIAL_SOURCES
    ${SERIAL_SOURCE_DIR}/serial.cc
)

if (APPLE)
    find_library(SERIAL_IOKIT_LIBRARY IOKit)
    find_library(SERIAL_FOUNDATION_LIBRARY Foundation)

    list(APPEND SERIAL_SOURCES
        ${SERIAL_SOURCE_DIR}/impl/unix.cc
        ${SERIAL_SOURCE_DIR}/impl/list_ports/list_ports_osx.cc
    )
elseif(UNIX)
    list(APPEND SERIAL_SOURCES
        ${SERIAL_SOURCE_DIR}/impl/unix.cc
        ${SERIAL_SOURCE_DIR}/impl/list_ports/list_ports_linux.cc
    )
else()
    list(APPEND SERIAL_SOURCES
        ${SERIAL_SOURCE_DIR}/impl/win.cc
        ${SERIAL_SOURCE_DIR}/impl/list_ports/list_ports_win.cc
    )
endif()

if (NOT EXISTS "${SERIAL_INCLUDE_DIR}")
    message(FATAL_ERROR "serial include path not found: ${SERIAL_INCLUDE_DIR}")
endif()

foreach(SERIAL_SOURCE ${SERIAL_SOURCES})
    if (NOT EXISTS "${SERIAL_SOURCE}")
        message(FATAL_ERROR "serial source file not found: ${SERIAL_SOURCE}")
    endif()
endforeach()

if (TARGET my_serial_core)
    print_colored_message("Target my_serial_core already exists. ✅" COLOR green)
else()
    add_library(my_serial_core STATIC ${SERIAL_SOURCES})
    target_include_directories(my_serial_core PUBLIC ${SERIAL_INCLUDE_DIR})

    if (APPLE)
        target_link_libraries(my_serial_core PUBLIC ${SERIAL_FOUNDATION_LIBRARY} ${SERIAL_IOKIT_LIBRARY})
    elseif(UNIX)
        target_link_libraries(my_serial_core PUBLIC rt pthread)
    else()
        target_link_libraries(my_serial_core PUBLIC setupapi)
    endif()

    set_target_properties(my_serial_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
    print_colored_message("serial configured: my_serial_core ✅" COLOR green)
endif()

message(STATUS "添加 serial 包含路径到 THIRD_PARTY_INCLUDES")
print_colored_message("<include> Found serial include" COLOR green)

if (DEFINED THIRD_INCLUDE_DIRECTORIES)
    list(APPEND THIRD_INCLUDE_DIRECTORIES ${SERIAL_INCLUDE_DIR})
    list(REMOVE_DUPLICATES THIRD_INCLUDE_DIRECTORIES)
endif()

list(APPEND THIRD_PARTY_INCLUDES ${SERIAL_INCLUDE_DIR})
list(REMOVE_DUPLICATES THIRD_PARTY_INCLUDES)

print_colored_message("Setting up serial over." COLOR yellow)
print_colored_message("------------------------------" COLOR magenta)