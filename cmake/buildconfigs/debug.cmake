set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)

find_program(CMAKE_C_COMPILER gcc)
find_program(CMAKE_CXX_COMPILER g++)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "Cannot find C compiler")
endif()

if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "Cannot find C++ compiler")
endif()
