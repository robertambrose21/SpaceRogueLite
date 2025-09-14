cmake_minimum_required(VERSION 3.15)
project(server CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(EnTT REQUIRED)

add_executable(server src/main.cpp)

target_link_libraries(server PRIVATE EnTT::EnTT)

install(TARGETS server DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
