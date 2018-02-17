cmake_minimum_required (VERSION 3.5.1)
project (HeartsNN)

add_definitions(-Wall -std=c++11 -march=native)

add_executable (analyze analyze.cpp)
add_executable (deal deal.cpp)
add_executable (disttest disttest.cpp)
add_executable (hearts hearts.cpp)
add_executable (tournament tournament.cpp)
add_executable (validate validate.cpp)

include(tensorflow.cmake)

add_subdirectory (lib)
include_directories( ${PROJECT_SOURCE_DIR} )

target_link_libraries(analyze lib ${TensorFlow_LIBRARIES})
target_link_libraries(deal lib ${TensorFlow_LIBRARIES})
target_link_libraries(disttest lib ${TensorFlow_LIBRARIES})
target_link_libraries(hearts lib ${TensorFlow_LIBRARIES})
target_link_libraries(tournament lib ${TensorFlow_LIBRARIES})
target_link_libraries(validate lib ${TensorFlow_LIBRARIES})
