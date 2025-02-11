cmake_minimum_required(VERSION 3.28)
project(Nexus)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(PLATFORM_WIN32 _WINSOCK_DEPRECATED_NO_WARNINGS DEBUG)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -static-libstdc++")

include_directories(.)
add_executable(Nexus src/main.cpp
        src/net/socket.cpp
        src/net/http_server.cpp
        src/platform/win32/win32_net.cpp
        include/net/http_resolver.h
        include/net/http_connection.h
        include/net/http_handler.h
        include/io/resource_locator.h
        include/net/https_server.h
        src/net/https_server.cpp
        include/net/https_connection.h
        include/log/logger.h
        include/io/terminal.h
        src/platform/win32/win32_io.cpp
        include/parallel/worker.h
        include/net/basic_handlers.h
        src/net/basic_handlers.cpp
)
add_executable(NexusTest test/test.cpp
        src/net/socket.cpp
        src/platform/win32/win32_net.cpp
        src/net/http_server.cpp
        include/net/http_resolver.h
        include/net/http_handler.h
        src/net/https_server.cpp
)
set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
find_package(OpenSSL REQUIRED)
target_link_libraries(Nexus ws2_32 dbghelp Dnsapi OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(NexusTest ws2_32 Dnsapi)