# set(DOCUMENTATION OFF CACHE BOOL "Disable documentation generation")
# set(WITH_STATIC_LIBRARIES ON CACHE BOOL "" FORCE)
# # 开启共享库以供 CPack 识别，或者只用静态库（根据你的需求）
# set(WITH_SHARED_LIBRARIES ON CACHE BOOL "" FORCE) 
# add_subdirectory(external/mosquitto EXCLUDE_FROM_ALL)

set(DOCUMENTATION OFF CACHE BOOL "Disable documentation generation")
add_subdirectory(external/mosquitto)