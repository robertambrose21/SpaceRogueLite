cmake_minimum_required(VERSION 3.15)
project(client CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(core REQUIRED)
find_package(net REQUIRED)
find_package(yojimbo REQUIRED)
find_package(graphics REQUIRED)

add_executable(client
        src/main.cpp
        src/net/client.cpp
        src/net/clientmessagehandler.cpp
        src/net/clientmessagetransmitter.cpp
        src/net/inputcommandhandler.cpp
        src/net/commandparser.h
        src/net/commandqueue.h)
set_target_properties(client PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(client PRIVATE core::core net::net yojimbo::yojimbo graphics::graphics)

install(TARGETS client DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
