cmake_minimum_required(VERSION 3.15)
project(net LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(yojimbo REQUIRED)

add_library(net INTERFACE)

target_include_directories(net
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(net_dummy OBJECT dummy.cpp)

target_include_directories(net_dummy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${yojimbo_INCLUDE_DIRS}
)

install(DIRECTORY include/ DESTINATION include)

install(TARGETS net
    EXPORT netTargets
    INCLUDES DESTINATION include
)

install(EXPORT netTargets
    FILE netTargets.cmake
    NAMESPACE net::
    DESTINATION lib/cmake/net
)