set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)

find_program(CMAKE_C_COMPILER gcc)
find_program(CMAKE_CXX_COMPILER g++)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "Cannot find C compiler")
endif()

if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "Cannot find C++ compiler")
endif()

set(
    CMAKE_C_COMPILER
    "${CMAKE_C_COMPILER}"
    CACHE
    STRING
    "C compiler"
    FORCE
)

set(
    CMAKE_CXX_COMPILER
    "${CMAKE_CXX_COMPILER}"
    CACHE
    STRING
    "C++ compiler"
    FORCE
)
