find_package(TensorflowCC REQUIRED COMPONENTS Shared)

get_target_property(TensorflowCC_INCLUDES TensorflowCC::Shared INTERFACE_INCLUDE_DIRECTORIES)
echovar(TensorflowCC_INCLUDES)

# target_link_libraries(example TensorflowCC::Static)
