add_executable(cliclient cliclient.cpp $<TARGET_OBJECTS:play_hearts_lib>)

target_link_libraries(cliclient
    core_lib
    gRPC::gpr
    gRPC::grpc++
    )
