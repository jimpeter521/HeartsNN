cmake_minimum_required (VERSION 3.6.2)
project (HeartsNN)

add_definitions(-Wall -std=c++11 -march=native)

include(tensorflow.cmake)

add_executable (hearts hearts.cpp)

add_subdirectory (lib)
include_directories( ${PROJECT_SOURCE_DIR} )

target_link_libraries(hearts ${TensorFlow_LIBRARIES} lib)

install(TARGETS hearts DESTINATION bin)
