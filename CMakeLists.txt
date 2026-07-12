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

set(EXTERNAL_SOURCES
    external/glad/glad.c
    external/glad/glad.h
    external/KHR/khrplatform.h
)

# Build toybox
add_executable(toybox
    ${IMGUI_SOURCES}
    ${EXTERNAL_SOURCES}

    src/main.cpp

    src/Parsers/DATParser.hpp
    src/Parsers/DATParser.cpp
    src/Parsers/BinaryReader.hpp
    src/Parsers/BinaryReader.cpp
    src/Parsers/TextureParser.cpp
    src/Parsers/GeomParser.cpp
    src/Parsers/NGNParser.cpp
    src/Parsers/RAWParser.cpp
    src/Parsers/ALLParser.hpp
    src/Parsers/ALLParser.cpp

    src/FileUtils.hpp
    src/Globals.hpp
    src/Common.hpp

    src/Clock.hpp
    src/Clock.cpp

    src/Wireframe.hpp
    src/Wireframe.cpp

    src/Shader.hpp
    src/Shader.cpp
)

target_include_directories(toybox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(toybox PRIVATE
    glfw
    OpenGL::GL
)