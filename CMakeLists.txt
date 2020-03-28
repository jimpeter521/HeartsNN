# We don't strictly require a version of cmake as recent as this.
# But the only build system that is fully supported requires Docker,
# and the standard Docker build image installs this version of cmake
cmake_minimum_required (VERSION 3.16.2)

project (HeartsNN)

set(CMAKE_CXX_STANDARD 17)

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

add_subdirectory(websockets)
