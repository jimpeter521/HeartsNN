add_executable(server
    play_hearts_server.cpp
    PlayerSession.cpp
    ClientPlayer.cpp
    $<TARGET_OBJECTS:play_hearts_lib>)

target_link_libraries(server
    core_lib
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    )
