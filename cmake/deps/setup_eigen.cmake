message("设置Eigen依赖")
set(EIGEN3_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/eigen)

if (NOT EXISTS ${CMAKE_SOURCE_DIR}/external/eigen)
    message(FATAL_ERROR "Eigen3 directory not found in external/eigen")
else()
    message(STATUS "Found Eigen3 include directory: ${EIGEN3_INCLUDE_DIR}")
    list(APPEND THIRD_INCLUDE_DIRECTORIES ${EIGEN3_INCLUDE_DIR})
endif()

list(APPEND THIRD_PARTY_INCLUDES ${EIGEN3_INCLUDE_DIR})