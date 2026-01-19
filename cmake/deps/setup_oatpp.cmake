# cmake/setup_oatpp.cmake

# 1. 基础配置
set(OATPP_BUILD_TESTS ON CACHE BOOL "" FORCE)
set(OATPP_INSTALL OFF CACHE BOOL "" FORCE)

# 2. 编译核心
add_subdirectory(external/oatpp)

# 3. 内存伪造与 Config 文件模拟
set(oatpp_VERSION "1.4.0")
set(oatpp_FOUND TRUE)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/external/oatpp/oatppConfig.cmake" "# Dummy")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/external/oatpp/oatppConfigVersion.cmake" 
     "set(PACKAGE_VERSION \"1.4.0\")\nset(PACKAGE_VERSION_COMPATIBLE TRUE)")
set(oatpp_DIR "${CMAKE_CURRENT_BINARY_DIR}/external/oatpp")

# 4. 别名建立
if(TARGET oatpp)
    add_library(oatpp::oatpp ALIAS oatpp)
endif()
if(TARGET oatpp-test)
    add_library(oatpp::oatpp-test ALIAS oatpp-test)
endif()

# 5. 编译 Swagger
add_subdirectory(external/oatpp-swagger)

# 6. Swagger 别名
if(TARGET oatpp-swagger)
    add_library(oatpp::oatpp-swagger ALIAS oatpp-swagger)
endif()

# # 假设你原来的 setup_oatpp.cmake 内容，这里确保包含 swagger
# # 确保你已经 clone 了 git 仓库到 external/oatpp 和 external/oatpp-swagger
# option(OATPP_BUILD_TESTS "Oat++ build tests" OFF)
# add_subdirectory(external/oatpp EXCLUDE_FROM_ALL)
# add_subdirectory(external/oatpp-swagger EXCLUDE_FROM_ALL)
# list(APPEND THIRD_PARTY_INCLUDES 
#     ${CMAKE_SOURCE_DIR}/external/oatpp/src
#     ${CMAKE_SOURCE_DIR}/external/oatpp-swagger/src
# )