cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(mem_jit CXX)

add_library(${PROJECT_NAME} STATIC
    src/sljit/sljit_src/sljitLir.cpp
    src/sljit/sljit_src/sljitLir.h
    src/mem_jit_pattern.cpp
    include/mem/mem_jit_pattern.h)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    vendor/mem
    src/sljit/sljit_src
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
