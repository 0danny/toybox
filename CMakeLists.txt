cmake_minimum_required(VERSION 4.2.0)

set(PROJECT_NAME "toybox")

project(${PROJECT_NAME} CXX)

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
    src/par_Tex.cpp
    src/par_Geom.cpp
    src/par_NGN.cpp
    src/file.cpp
    src/structs.h
)

target_include_directories(toybox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(toybox PRIVATE
    glfw
    OpenGL::GL
)