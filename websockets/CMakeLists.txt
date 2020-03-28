add_subdirectory(uWebSockets)

add_executable(webSocketServer webSocketServer.cpp CardsWebServer.cpp)
target_link_libraries(webSocketServer uWebSockets stdc++fs core_lib pthread)

# TODO: use OPTIONs
target_compile_definitions(webSocketServer PUBLIC LIBUS_NO_SSL)
target_compile_definitions(webSocketServer PUBLIC UWS_NO_ZLIB)
target_compile_definitions(webSocketServer PUBLIC WITH_LIBUV)
