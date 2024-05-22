
# This file is a part of MRNIU/fdt-parser
# (https://github.com/MRNIU/fdt-parser).
#
# CMakeLists.txt for MRNIU/fdt-parser.

# Set minimum cmake version
cmake_minimum_required(VERSION 3.10)

project(fdt_parser
    LANGUAGES CXX
    VERSION 0.0.1
)

# 禁止原地编译
if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(
            FATAL_ERROR
            "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif ()

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCE_DIR}/src/fdt_parser.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-O3;-Werror>
        -Wall
        -Wextra
        # -ffreestanding
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/fdt_parser.h")

if (CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif ()
