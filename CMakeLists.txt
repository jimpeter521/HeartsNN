cmake_minimum_required (VERSION 3.6.2)
project (HeartsNN)

add_definitions(-Wall -std=c++11 -march=native)

include(tensorflow.cmake)

add_executable (analyze analyze.cpp)
add_executable (deal deal.cpp)
add_executable (disttest disttest.cpp)
add_executable (hearts hearts.cpp)
add_executable (tournament tournament.cpp)
add_executable (validate validate.cpp)

add_subdirectory (lib)
include_directories( ${PROJECT_SOURCE_DIR} )

target_link_libraries(analyze ${TensorFlow_LIBRARIES} lib)
target_link_libraries(deal ${TensorFlow_LIBRARIES} lib)
target_link_libraries(disttest ${TensorFlow_LIBRARIES} lib)
target_link_libraries(hearts ${TensorFlow_LIBRARIES} lib)
target_link_libraries(tournament ${TensorFlow_LIBRARIES} lib)
target_link_libraries(validate ${TensorFlow_LIBRARIES} lib)
