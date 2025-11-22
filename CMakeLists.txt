cmake_minimum_required(VERSION 3.15)
project(all CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(graphics)
add_subdirectory(client)
add_subdirectory(server)