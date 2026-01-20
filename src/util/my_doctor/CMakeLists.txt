cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_MY_DOCTOR)
    message("BUILD_MY_DOCTOR is ON")
    print_colored_message("------------------------------" COLOR magenta)
    print_colored_message("Building my_doctor library..." COLOR yellow)
    
    set(MY_DOCTOR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/util/my_doctor)
    file(GLOB_RECURSE MY_DOCTOR_SOURCES ${PROJECT_SOURCE_DIR}/src/util/my_doctor/*.cpp)

    pretty_print_list("MYCONFIG_INCLUDE_DIRECTORIES List" MY_DOCTOR_INCLUDE_DIRECTORIES)
    pretty_print_list("MYCONFIG_SOURCES List" MY_DOCTOR_SOURCES)

    add_library(my_doctor STATIC ${MY_DOCTOR_SOURCES})
    target_include_directories(my_doctor PUBLIC ${MY_DOCTOR_INCLUDE_DIRECTORIES})
    target_link_libraries(my_doctor PUBLIC mylog)
    target_link_libraries(my_doctor PUBLIC myconfig)
    target_link_libraries(my_doctor PUBLIC my_edge)
    target_link_libraries(my_doctor PUBLIC oatpp::oatpp)
    target_link_libraries(my_doctor PUBLIC oatpp::oatpp-swagger)
    print_colored_message("Building my_doctor library over." COLOR yellow)
    print_colored_message("------------------------------" COLOR magenta)
else()
    message("BUILD_MY_DOCTOR is OFF, skipping my_config library build")
    return()
endif()

