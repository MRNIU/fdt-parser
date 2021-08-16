
# This file is a part of MRNIU/fdt-parser
# (https://github.com/MRNIU/fdt-parser).
#
# CMakeLists.txt for MRNIU/fdt-parser.

# Set minimum cmake version
cmake_minimum_required(VERSION 3.10)

project(fdt_parser LANGUAGES CXX)

if(${PROJECT_SOURCE_DIR} STREQUAL ${fdt_parser_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

set(CMAKE_CXX_STANDARD 17)

# Set cmake moudle path
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/ fdr_parser_src)

include_directories(${PROJECT_SOURCE_DIR}/src/include/)

add_library(fdt_parser SHARED ${fdr_parser_src})
add_library(fdt_parser_static STATIC ${fdr_parser_src})
