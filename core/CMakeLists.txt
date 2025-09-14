cmake_minimum_required(VERSION 3.15)
project(core CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(EnTT REQUIRED)

add_library(core src/game.cpp)
target_link_libraries(core PUBLIC EnTT::EnTT)
target_include_directories(core PUBLIC include)



set_target_properties(core PROPERTIES PUBLIC_HEADER "include/game.h")
install(TARGETS core)