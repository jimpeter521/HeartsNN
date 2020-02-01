find_path(TensorFlow_INCLUDE_DIR
        NAMES
        tensorflow/core
        tensorflow/cc
        third_party
        HINTS
        /usr/local/include/tensorflow)

include(FindPackageHandleStandardArgs)
unset(TENSORFLOW_FOUND)

find_library(TensorFlow__framework_LIBRARY NAMES tensorflow_framework
        HINTS
        /usr/local/lib)

find_library(TensorFlow_cc_LIBRARY NAMES tensorflow_cc
        HINTS
        /usr/local/lib)

# set TensorFlow_FOUND
find_package_handle_standard_args(TensorFlow DEFAULT_MSG TensorFlow_INCLUDE_DIR TensorFlow_cc_LIBRARY TensorFlow__framework_LIBRARY)

# set external variables for usage in CMakeLists.txt
if(TENSORFLOW_FOUND)
    set(TensorFlow_LIBRARIES ${TensorFlow_cc_LIBRARY} ${TensorFlow__framework_LIBRARY})
    set(TensorFlow_INCLUDE_DIRS ${TensorFlow_INCLUDE_DIR})
endif()

include_directories( ${TensorFlow_INCLUDE_DIRS} )
