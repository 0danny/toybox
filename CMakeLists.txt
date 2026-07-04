cmake_minimum_required(VERSION 4.2.0)

set(PROJECT_NAME "toybox")

project(${PROJECT_NAME} C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Packages
find_package(OpenGL REQUIRED)

# Build GLFW
add_subdirectory(external/glfw)

# Build ImGUI
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Build toybox
add_executable(toybox
    ${IMGUI_SOURCES}

    src/main.cpp

    src/parse/par_Tex.cpp
    src/parse/par_Geom.cpp
    src/parse/par_NGN.cpp
    src/parse/par_RAW.cpp
    src/parse/par_ALL.cpp

    src/file.cpp
    src/wireframe.cpp
    src/shader.cpp

    "external/glad/glad.c"
)

target_sources(toybox PRIVATE 
    include/common.hpp
    include/wireframe.hpp
    include/globals.hpp

    include/RAW.hpp
    include/NGN.hpp
    include/ALL.hpp

    external/glad/glad.h
    external/KHR/khrplatform.h
)

target_include_directories(toybox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(toybox PRIVATE
    glfw
    OpenGL::GL
)