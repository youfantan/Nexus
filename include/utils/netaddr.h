#pragma once

#include <cstdint>
#include <string>
#include "../mem/memory.h"
#include "../base/def.h"

#ifdef PLATFORM_WIN32
#include "../platform/win32/win32_net.h"
#endif

namespace Nexus::Utils {
    class NetAddr {
    private:
        char inaddr_[sizeof(in6_addr)] {};
        uint16_t port_ {0};
        SockType type_ {SockType::INVALID};
        std::string raw_addr_ {"invalid"};
    public:
        NetAddr() = default;
        NetAddr(const NetAddr& addr) : port_(addr.port_), type_(addr.type_), raw_addr_(addr.raw_addr_) {
            memcpy(inaddr_, addr.inaddr_, sizeof(in6_addr));
        }
        explicit NetAddr(const std::string& addr, uint16_t port) : raw_addr_(addr), port_(htons(port)) {
            in_addr v4adr{};
            in6_addr v6adr{};
            if (inet_pton(AF_INET, addr.c_str(), &v4adr) == 1) {
                //matched ipv4
                type_ = SockType::SOCK_IPV4;
                memcpy(inaddr_, &v4adr, sizeof(in_addr));
            } else if (inet_pton(AF_INET6, addr.c_str(), &v6adr) ==1) {
                //matched ipv6
                type_ = SockType::SOCK_IPV6;
                memcpy(inaddr_, &v6adr, sizeof(in6_addr));
            } else {
                //may be domain or unexpected input, try to query IPv4 address first
                type_ = SockType::SOCK_IPV4;
                auto v4 = DNSLookUpV4(addr);
                if (v4.is_valid()) {
                    memcpy(inaddr_, reinterpret_cast<char*>(&v4.reference()), sizeof(v4.reference()));
                } else {
                    //try IPV6 then
                    type_ = SockType::SOCK_IPV6;
                    auto v6 = DNSLookUpV6(addr);
                    if (v6.is_valid()) {
                        memcpy(inaddr_, reinterpret_cast<char*>(&v6.reference()), sizeof(v6.reference()));
                    } else {
                        //invalid_ address
                        type_ = SockType::INVALID;
                        BREAKPOINT;
                    }
                }
            }
        }

        explicit NetAddr(sockaddr* addr) {
            char ip[INET6_ADDRSTRLEN];
            if (addr->sa_family == AF_INET) {
                auto* sin = reinterpret_cast<sockaddr_in*>(addr);
                if (inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip)) == nullptr) {
                    raw_addr_ = "invalid address";
                    port_ = sin->sin_port;
                    type_ = SockType::SOCK_IPV4;
                } else {
                    raw_addr_ = ip;
                    port_ = sin->sin_port;
                    type_ = SockType::SOCK_IPV4;
                }

            } else if (addr->sa_family == AF_INET6) {
                auto* sin = reinterpret_cast<sockaddr_in6*>(addr);
                if (inet_ntop(AF_INET6, &sin->sin6_addr, ip, sizeof(ip)) == nullptr) {
                    raw_addr_ = "invalid address";
                    port_ = sin->sin6_port;
                    type_ = SockType::SOCK_IPV6;
                } else {
                    raw_addr_ = ip;
                    port_ = sin->sin6_port;
                    type_ = SockType::SOCK_IPV6;
                }

            } else {
                raw_addr_ = "invalid address";
                port_ = 0;
                type_ = SockType::INVALID;
            }
        }

        SockType type() { return type_; }

        Base::UniqueFlexHolder<sockaddr> addr() {
            Base::UniqueFlexHolder<sockaddr> holder(sizeof(sockaddr_in6));
            if (type_ == SockType::SOCK_IPV4) {
                sockaddr_in sa{};
                memset(&sa, 0, sizeof(sockaddr_in));
                sa.sin_family = AF_INET;
                sa.sin_port = port_;
                memcpy(&sa.sin_addr.S_un.S_addr, inaddr_, sizeof(in_addr));
                holder.assign(&sa);
            } else if (type_ == SockType::SOCK_IPV6) {
                sockaddr_in6 sa{};
                memset(&sa, 0, sizeof(sockaddr_in6));
                sa.sin6_family = AF_INET6;
                sa.sin6_port = port_;
                memcpy(reinterpret_cast<char*>(sa.sin6_addr.u.Byte), inaddr_, sizeof(in6_addr));
                holder.assign(&sa);
            }
            return holder;
        }

        Base::UniqueFlexHolder<sockaddr_in> addrv4() {
            Base::UniqueFlexHolder<sockaddr_in> holder(sizeof(sockaddr_in));
            sockaddr_in sa4{};
            memset(&sa4, 0, sizeof(sockaddr_in));
            sa4.sin_family = AF_INET;
            memcpy(reinterpret_cast<char*>(&sa4.sin_addr.S_un.S_addr), inaddr_, sizeof(in_addr));
            sa4.sin_port = port_;
            holder.assign(&sa4);
            return holder;
        }

        Base::UniqueFlexHolder<sockaddr_in6> addrv6() {
            Base::UniqueFlexHolder<sockaddr_in6> holder(sizeof(sockaddr_in6));
            sockaddr_in6 sa6{};
            memset(&sa6, 0, sizeof(sockaddr_in6));
            sa6.sin6_family = AF_INET6;
            memcpy(reinterpret_cast<char*>(sa6.sin6_addr.u.Byte), inaddr_, sizeof(in6_addr));
            sa6.sin6_port = port_;
            holder.assign(&sa6);
            return holder;
        }

        int size() {
            if (type_ == SockType::SOCK_IPV4) return sizeof(sockaddr_in);
            else if (type_ == SockType::SOCK_IPV6) return sizeof(sockaddr_in6);
                else return 0;
        }

        uint16_t port() {
            return port_;
        }

        std::string url() {
            return std::format("{}:{}", raw_addr_, ntohs(port_));
        }
    };
}