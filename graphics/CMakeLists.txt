cmake_minimum_required(VERSION 3.15)
project(graphics CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(imgui REQUIRED)
find_package(core REQUIRED)

add_library(graphics
    src/window.cpp
    include/backends/imgui_impl_sdl3.cpp
    include/backends/imgui_impl_sdlgpu3.cpp
)
set_target_properties(graphics PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(graphics PUBLIC SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf-static imgui::imgui core::core)
target_include_directories(graphics PUBLIC include)

set_target_properties(graphics PROPERTIES PUBLIC_HEADER "include/window.h")
install(TARGETS graphics)
install(DIRECTORY include/backends DESTINATION include)
