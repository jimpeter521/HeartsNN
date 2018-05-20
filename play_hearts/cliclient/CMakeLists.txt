add_executable(cliclient cliclient.cpp $<TARGET_OBJECTS:play_hearts_lib>)

target_link_libraries(cliclient
    ${ALL_LIBRARIES}
    gRPC::grpc++_reflection
    protobuf::libprotobuf)
