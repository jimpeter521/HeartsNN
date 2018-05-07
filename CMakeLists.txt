cmake_minimum_required (VERSION 3.5.1)

include("gate/cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.20.61.tar.gz"
    SHA1 "b66f77ffdf97d9b318b86817b79991f4c45d3de1"
)

project (HeartsNN)

add_definitions(-march=native)
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(helloworld)

hunter_add_package(dlib)
find_package(dlib CONFIG REQUIRED)

include(tensorflow.cmake)

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
