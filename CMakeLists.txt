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

add_executable(analyze analyze.cpp)
add_executable(deal deal.cpp)
add_executable(disttest disttest.cpp)
add_executable(hearts hearts.cpp)
add_executable(tournament tournament.cpp)
add_executable(validate validate.cpp)
add_executable(numpywriter numpywriter.cpp)
add_executable(play play.cpp)

target_link_libraries(analyze core_lib)
target_link_libraries(deal core_lib)
target_link_libraries(disttest core_lib)
target_link_libraries(hearts core_lib)
target_link_libraries(tournament core_lib)
target_link_libraries(validate core_lib)
target_link_libraries(numpywriter core_lib)
target_link_libraries(play core_lib)

add_subdirectory(play_hearts)
