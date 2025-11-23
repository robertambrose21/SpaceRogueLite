cmake_minimum_required(VERSION 3.15)
project(graphics CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED)
find_package(imgui REQUIRED)
find_package(core REQUIRED)

add_library(graphics
    src/window.cpp
    src/backends/imgui_impl_sdl3.cpp
    src/backends/imgui_impl_sdlgpu3.cpp
)
set_target_properties(graphics PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(graphics PUBLIC SDL3::SDL3 imgui::imgui core::core)
target_include_directories(graphics PUBLIC include)

# Manually add imgui include directory (Conan package doesn't export it properly)
if(DEFINED imgui_PACKAGE_FOLDER_DEBUG)
    target_include_directories(graphics PRIVATE "${imgui_PACKAGE_FOLDER_DEBUG}/include")
endif()
if(DEFINED imgui_PACKAGE_FOLDER_RELEASE)
    target_include_directories(graphics PRIVATE "${imgui_PACKAGE_FOLDER_RELEASE}/include")
endif()

set_target_properties(graphics PROPERTIES PUBLIC_HEADER "include/window.h")
install(TARGETS graphics)
