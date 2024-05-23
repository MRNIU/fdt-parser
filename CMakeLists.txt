
# This file is a part of MRNIU/fdt-parser
# (https://github.com/MRNIU/fdt-parser).
#
# CMakeLists.txt for MRNIU/fdt-parser.

cmake_minimum_required(VERSION 3.27)

project(fdt_parser
    LANGUAGES CXX
    VERSION 2.0.0
)

# 禁止原地编译
if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(
            FATAL_ERROR
            "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif ()

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    ${PROJECT_SOURCE_DIR}/src/include
)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR)
    add_executable(${PROJECT_NAME}_test
        test/test.cpp
    )

    target_compile_options(${PROJECT_NAME}_test PRIVATE
        -Wall
        -Wextra
    )

    # 添加要链接的库
    target_link_libraries(${PROJECT_NAME}_test PRIVATE
        fdt_parser
    )
endif ()
