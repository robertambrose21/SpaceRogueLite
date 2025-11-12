cmake_minimum_required(VERSION 3.15)
project(server CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(core REQUIRED)
find_package(net REQUIRED)
find_package(yojimbo REQUIRED)

add_executable(server src/main.cpp src/net/server.cpp src/net/servermessagehandler.cpp)
set_target_properties(server PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(server PRIVATE core::core net::net yojimbo::yojimbo)

install(TARGETS server DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
