#pragma once

#ifdef PLATFORM_WIN32
#include "../platform/win32/win32_defs.h"
#include "../platform/win32/win32_net.h"
#endif
#include "../base/netdefs.h"
#include <string>
#include "../mem/memory.h"

namespace Nexus::Net {
    class SocketBuffer;
    class Socket {
        friend class SocketBuffer;
    private:
        io_handle_t fd_;
        bool invalid_ {false};
        SockType type_;
    public:
        Socket(io_handle_t fd, SockType typ);
        explicit Socket(SockType typ);
        bool bind(const std::string& addr, uint16_t port);
        bool bind(sockaddr_in6 addrv6, uint16_t port);
        bool bind(sockaddr_in addrv4, uint16_t port);
        bool connect(const std::string& addr, uint16_t port);
        bool connect(sockaddr_in6 addrv6, uint16_t port);
        bool connect(sockaddr_in addrv4, uint16_t port);
        void listen();
        Socket accept();
        void close();
        bool setnonblocking();
        bool invalid();
    };

    extern void CloseSocket(io_handle_t handle);
    extern bool SetNonblockingSocket(io_handle_t handle);

    constexpr static uint64_t default_socket_buffer_size = 128 * 1024;

    class SocketBuffer {
    private:
        const Socket& sock_;
        Stream<UniquePool<>> istream_;
        Stream<UniquePool<>> ostream_;
        uint64_t rpos_{};
        uint64_t wpos_{};
    public:
        explicit SocketBuffer(const Socket& socket);
        SocketBuffer(SocketBuffer&) = delete;
        SocketBuffer(SocketBuffer&& buffer);
        bool update();
        Stream<UniquePool<>>& istream();
        Stream<UniquePool<>>& ostream();
    };
}

#include "../../src/io/socket.tpp"