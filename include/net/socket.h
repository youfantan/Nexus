#pragma once

#include <string>
#ifdef PLATFORM_WIN32
#include "include/platform/win32/win32_defs.h"
#include "include/platform/win32/win32_net.h"
#endif
#include "include/base/netdefs.h"
#include "include/mem/memory.h"
#include "include/utils/netaddr.h"

namespace Nexus::Net {
    class Socket {
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
        bool bind(Nexus::Utils::NetAddr addr);
        bool connect(const std::string& addr, uint16_t port);
        bool connect(sockaddr_in6 addrv6, uint16_t port);
        bool connect(sockaddr_in addrv4, uint16_t port);
        void listen();
        Socket accept();
        void close();
        bool setnonblocking();
        bool invalid();
        io_handle_t fd();
    };

    extern void CloseSocket(io_handle_t handle);
    extern bool SetNonblockingSocket(io_handle_t handle);
}