cmake_minimum_required(VERSION 3.28)
project(Nexus)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(PLATFORM_WIN32 _WINSOCK_DEPRECATED_NO_WARNINGS)
include_directories(.)
add_executable(Nexus
        include/mem/memory.h
        src/mem/memory.tpp
        include/utils/check.h
        include/io/mux.h
        include/platform/win32/win32_defs.h
        include/platform/win32/win32_io.h
        include/io/socket.h
        include/utils/netaddr.h
        include/platform/win32/win32_net.h
        include/utils/unexpected.h
        include/platform/win32/win32_stacktrace.h
        src/io/socket.tpp
        include/base/netdefs.h
        main.cpp
)
target_link_libraries(Nexus ws2_32 Dnsapi)