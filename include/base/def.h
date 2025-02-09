#pragma once

enum class SockType {
    SOCK_IPV4,
    SOCK_IPV6,
    SOCK_UNIX,
    INVALID
};

constexpr int CPU_CORES = 1;