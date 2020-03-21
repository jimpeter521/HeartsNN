cmake_minimum_required (VERSION 3.10.2)

project (HeartsNN)

macro(echovar)
message("${ARGV0}: ${${ARGV0}} in context ${CMAKE_CURRENT_LIST_FILE}")
endmacro(echovar)

add_definitions(-march=native)
include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(dlib)
include(cmake/tensorflow.cmake)

add_subdirectory(lib)
add_subdirectory(apps)
add_subdirectory(play_hearts)
add_subdirectory(googletest)
add_subdirectory(tests)

add_subdirectory(websockets-cards)
