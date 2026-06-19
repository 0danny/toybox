cmake_minimum_required(VERSION 3.20.0)

set(PROJECT_NAME "toybox")

project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build toybox
add_executable(toybox
    src/main.cpp
)

target_include_directories(toybox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/include
)