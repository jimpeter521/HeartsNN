cmake_minimum_required (VERSION 3.16.2)

find_program(CMAKE_C_COMPILER gcc)
find_program(CMAKE_CXX_COMPILER g++)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "Cannot find gcc")
endif()

if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "Cannot find g++")
endif()

set(CMAKE_CXX_STANDARD 17)

project (socketio-cards)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(uWebSockets)

add_executable(webSocketServer webSocketServer.cpp CardsWebServer.cpp)
target_link_libraries(webSocketServer uWebSockets stdc++fs pthread)

# TODO: use OPTIONs
target_compile_definitions(webSocketServer PUBLIC LIBUS_NO_SSL)
target_compile_definitions(webSocketServer PUBLIC UWS_NO_ZLIB)
target_compile_definitions(webSocketServer PUBLIC WITH_LIBUV)
