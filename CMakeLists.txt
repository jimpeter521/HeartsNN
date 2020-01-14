cmake_minimum_required (VERSION 3.10.2)

project (HeartsNN)

add_definitions(-march=native)
include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(TensorFlow_INCLUDE_DIR /usr/local/include/tensorflow)
# set(TensorFlow__framework_LIBRARY /usr/local/lib)
# set(PROTOBUF_INCLUDE_DIR /usr/local/include/tensorflow/bazel-tensorflow/external/org_tensorflow/tensorflow/core/platform)
# set(PROTOBUF_LIBRARY /usr/local/lib/tensorflow_cc/)
# set(PROTOBUF_PROTOC_EXECUTABLE /usr/local/lib/tensorflow_cc/)


include(cmake/dlib.cmake)
include(cmake/tensorflow.cmake)

add_executable(analyze analyze.cpp)
add_executable(deal deal.cpp)
add_executable(disttest disttest.cpp)
add_executable(hearts hearts.cpp)
add_executable(tournament tournament.cpp)
add_executable(validate validate.cpp)
add_executable(numpywriter numpywriter.cpp)
add_executable(play play.cpp)

add_subdirectory(lib)

set(ALL_LIBRARIES lib ${TensorFlow_LIBRARIES} dlib::dlib)

target_link_libraries(analyze ${ALL_LIBRARIES})
target_link_libraries(deal ${ALL_LIBRARIES})
target_link_libraries(disttest ${ALL_LIBRARIES})
target_link_libraries(hearts ${ALL_LIBRARIES})
target_link_libraries(tournament ${ALL_LIBRARIES})
target_link_libraries(validate ${ALL_LIBRARIES})
target_link_libraries(numpywriter ${ALL_LIBRARIES})
target_link_libraries(play ${ALL_LIBRARIES})

add_subdirectory(play_hearts)
