cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(mem_jit CXX)

set(ASMJIT_STATIC TRUE)
add_definitions(-DASMJIT_STATIC)

add_subdirectory(vendor)

file(GLOB ${PROJECT_NAME}_SRCS
    *.cpp
    *.h)

add_library(${PROJECT_NAME} STATIC
    ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
    vendor/asmjit/src
    vendor/mem
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
