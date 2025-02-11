#pragma once

enum class SockType {
    SOCK_IPV4,
    SOCK_IPV6,
    SOCK_UNIX,
    INVALID
};

constexpr int CPU_CORES = 2;

static inline constexpr std::string_view get_not_found_resp = "<html><body><h1>404 Not Found</h1><p>Server: Nexus@BetaV1</p></body></html>";
static inline constexpr std::string_view post_not_found_resp = "Handler Not Found | Nexus@BetaV1";