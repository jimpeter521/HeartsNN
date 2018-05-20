add_executable(testclient play_hearts_client.cpp $<TARGET_OBJECTS:play_hearts_lib>)

target_link_libraries(testclient
    ${ALL_LIBRARIES}
    gRPC::grpc++_reflection
    protobuf::libprotobuf)
