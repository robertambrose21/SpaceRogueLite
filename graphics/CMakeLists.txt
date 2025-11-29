cmake_minimum_required(VERSION 3.15)
project(graphics CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(imgui REQUIRED)
find_package(core REQUIRED)

# Include shader compilation utilities
include(cmake/CompileShaders.cmake)

# Compile shaders at build time
compile_shader_to_header(
    VERTEX_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_square.vert.hlsl
    FRAGMENT_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_square.frag.hlsl
    OUTPUT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/shaders/simple_square_shaders.h
    SHADER_NAME simple_square
)

compile_shader_to_header(
    VERTEX_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_quad.vert.hlsl
    FRAGMENT_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_quad.frag.hlsl
    OUTPUT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/shaders/textured_quad_shaders.h
    SHADER_NAME textured_quad
)

compile_shader_to_header(
    VERTEX_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tilecompose.vert.hlsl
    FRAGMENT_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tilecompose.frag.hlsl
    OUTPUT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/shaders/tilecompose_shaders.h
    SHADER_NAME tilecompose
)

add_library(graphics
    src/window.cpp
    src/camera.cpp
    src/tilemap.cpp
    src/tileatlas.cpp
    src/tilerenderer.cpp
    include/backends/imgui_impl_sdl3.cpp
    include/backends/imgui_impl_sdlgpu3.cpp
)
set_target_properties(graphics PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(graphics PUBLIC SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf-static imgui::imgui core::core)
target_include_directories(graphics
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

set_target_properties(graphics PROPERTIES PUBLIC_HEADER 
    "include/window.h", 
    "include/camera.h", 
    "include/tiletypes.h", 
    "include/tilemap.h", 
    "include/tileatlas.h", 
    "include/tilerenderer.h"
)
install(TARGETS graphics)
install(DIRECTORY include/backends DESTINATION include)
