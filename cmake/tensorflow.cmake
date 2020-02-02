find_package(TensorflowCC REQUIRED COMPONENTS Static)

get_target_property(TensorflowCC_INCLUDES TensorflowCC::Static INTERFACE_INCLUDE_DIRECTORIES)
echovar(TensorflowCC_INCLUDES)

# target_link_libraries(example TensorflowCC::Static)
