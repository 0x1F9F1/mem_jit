cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(mem_jit CXX)

set(ASMJIT_STATIC TRUE)
add_definitions(-DASMJIT_STATIC)

add_subdirectory(vendor EXCLUDE_FROM_ALL)

add_library(${PROJECT_NAME} STATIC
    src/jit_pattern.cpp
    include/mem/jit_pattern.h
)

target_include_directories(${PROJECT_NAME}
    PUBLIC include
    PUBLIC vendor/mem
    PRIVATE vendor/asmjit/src
)

target_link_libraries(${PROJECT_NAME}
    asmjit)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
