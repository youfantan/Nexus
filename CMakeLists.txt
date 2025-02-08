cmake_minimum_required(VERSION 3.28)
project(Nexus)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(PLATFORM_WIN32 _WINSOCK_DEPRECATED_NO_WARNINGS DEBUG)
set(CMAKE_CXX_FLAGS -static-libstdc++)
include_directories(.)
add_executable(Nexus src/main.cpp
        src/net/socket.cpp
        src/net/http_server.cpp
        src/platform/win32/win32_net.cpp
        include/net/http_resolver.h
        include/net/http_connection.h
        include/net/http_handler.h
        include/io/resource_locator.h
)
add_executable(NexusTest test/test.cpp
        src/net/socket.cpp
        src/platform/win32/win32_net.cpp
        src/net/http_server.cpp
        include/net/http_resolver.h
        include/net/http_handler.h
)
target_link_libraries(Nexus ws2_32 Dnsapi)
target_link_libraries(NexusTest ws2_32 Dnsapi)