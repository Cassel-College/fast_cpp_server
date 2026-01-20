# cmake/setup_coverage.cmake

print_colored_message("------------------------------" COLOR cyan)
print_colored_message("Configuring Code Coverage..." COLOR yellow)

# 1. 查找 gcovr 工具是否存在
find_program(GCOVR_EXE gcovr)

if(NOT GCOVR_EXE)
    print_colored_message("警告: 未找到 gcovr 工具，无法生成覆盖率报告。" COLOR red)
    print_colored_message("请安装 gcovr 并确保其在系统 PATH 中。" COLOR red)
    print_colored_message("apt-get install gcovr (Ubuntu/Debian)" COLOR red)
    return()
else()
    print_colored_message("环境中找到 gcovr: ${GCOVR_EXE}" COLOR green)
endif()

# 2. 定义覆盖率输出路径
set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
file(MAKE_DIRECTORY ${COVERAGE_OUTPUT_DIR})

# 动态确定依赖项：如果定义了测试目标，就依赖它
set(COVERAGE_DEPS ${PROJECT_NAME})
if(TARGET ${PROJECT_NAME}_Test)
    list(APPEND COVERAGE_DEPS ${PROJECT_NAME}_Test)
endif()

# # 3. 创建自定义目标 coverage（忽略第三方库路径错误）
# set(COVERAGE_DIR_NAME "coverage_report_ignore_external")
add_custom_target(coverage
    COMMAND ${GCOVR_EXE} 
            -r . 
            --filter "src/"                # 核心：只统计 src 目录
            --gcov-ignore-errors=no_working_dir_found  # 忽略第三方库路径解析错误
            --html --html-details 
            --output "build/${COVERAGE_DIR_NAME}/index.html"
            --print-summary
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${COVERAGE_DEPS}
    COMMENT "Generating HTML coverage report, ignoring external source errors..."
)

print_colored_message("Coverage target 'coverage' has been created." COLOR green)