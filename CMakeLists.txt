cmake_minimum_required (VERSION 3.11.1)

project (HeartsNN)

add_definitions(-march=native)
include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(tensorflow.cmake)

include(FetchContent)
FetchContent_Declare(dlib
    GIT_REPOSITORY https://github.com/davisking/dlib.git
    GIT_TAG        v19.18
)
FetchContent_MakeAvailable(dlib)

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
