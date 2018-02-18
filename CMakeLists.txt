cmake_minimum_required (VERSION 3.5.1)
project (HeartsNN)

add_definitions(-std=c++11 -march=native)

include(tensorflow.cmake)

add_executable(analyze analyze.cpp)
add_executable(deal deal.cpp)
add_executable(disttest disttest.cpp)
add_executable(hearts hearts.cpp)
add_executable(tournament tournament.cpp)
add_executable(validate validate.cpp)

include_directories( ${PROJECT_SOURCE_DIR} dlib )

add_subdirectory(dlib)
add_subdirectory(lib)

set(ALL_LIBRARIES lib dlib ${TensorFlow_LIBRARIES})

target_link_libraries(analyze ${ALL_LIBRARIES})
target_link_libraries(deal ${ALL_LIBRARIES})
target_link_libraries(disttest ${ALL_LIBRARIES})
target_link_libraries(hearts ${ALL_LIBRARIES})
target_link_libraries(tournament ${ALL_LIBRARIES})
target_link_libraries(validate ${ALL_LIBRARIES})
