cmake_minimum_required(VERSION 3.15)
project(graphics CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED)
find_package(core REQUIRED)

add_library(graphics src/window.cpp)
set_target_properties(graphics PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(graphics PUBLIC SDL3::SDL3 core::core)
target_include_directories(graphics PUBLIC include)

set_target_properties(graphics PROPERTIES PUBLIC_HEADER "include/window.h")
install(TARGETS graphics)
