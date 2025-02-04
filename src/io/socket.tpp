//#include "include/io/socket.h"
#include "include/utils/netaddr.h"


using namespace Nexus::Net;
using namespace Nexus::Utils;

Socket::Socket(SockType typ) : type_(typ) {
    if (typ == SockType::SOCK_IPV4) fd_ = socket(AF_INET, SOCK_STREAM, 0);
    else if (typ == SockType::SOCK_IPV6) fd_ = socket(AF_INET6, SOCK_STREAM, 0);
    else {
        invalid_ = true;
        fd_ = 0;
    }
}

Socket::Socket(io_handle_t fd, SockType typ) : fd_(fd), type_(typ), invalid_(false) {}

bool Socket::bind(const std::string &addr, uint16_t port) {
    if (invalid_) return false;
    NetAddr na(addr, port);
    if (na.type() != SockType::INVALID) {
        if (na.type() == SockType::SOCK_IPV4 && type_ != SockType::SOCK_IPV4) {
            invalid_ = true;
            return false;
        } else if (na.type() == SockType::SOCK_IPV6 && type_ != SockType::SOCK_IPV6) {
            invalid_ = true;
            return false;
        }
        int r = ::bind(fd_, na.addr().ptr(), na.size());
        if (r == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            } else {
                invalid_ = true;
                return false;
            }
        }
        return true;
    } else {
        invalid_ = true;
        return false;
    }
}

bool Socket::bind(sockaddr_in6 addrv6, uint16_t port) {
    if (type_ != SockType::SOCK_IPV6) {
        invalid_ = true;
        return false;
    }
    int r = ::bind(fd_, reinterpret_cast<sockaddr*>(&addrv6), sizeof(sockaddr_in6));
    if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        } else {
            invalid_ = true;
            return false;
        }
    }
    return true;
}

bool Socket::bind(sockaddr_in addrv4, uint16_t port) {
    if (type_ != SockType::SOCK_IPV4) {
        invalid_ = true;
        return false;
    }
    int r = ::bind(fd_, reinterpret_cast<sockaddr*>(&addrv4), sizeof(sockaddr_in));
    if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        } else {
            invalid_ = true;
            return false;
        }
    }
    return true;
}

bool Socket::connect(const std::string &addr, uint16_t port) {
    NetAddr na(addr, port);
    int r = ::connect(fd_, na.addr().ptr(), na.size());
    if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool Socket::connect(sockaddr_in6 addrv6, uint16_t port) {
    int r = ::connect(fd_, reinterpret_cast<sockaddr*>(&addrv6), sizeof(sockaddr_in6));
    if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool Socket::connect(sockaddr_in addrv4, uint16_t port) {
    int r = ::connect(fd_, reinterpret_cast<sockaddr*>(&addrv4), sizeof(sockaddr_in));
    if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

void Socket::listen() {
    ::listen(fd_, 1024);
}

Socket Socket::accept() {
    sockaddr_storage ss{};
    int len = sizeof(sockaddr_storage);
    io_handle_t sock = ::accept(fd_, reinterpret_cast<sockaddr*>(&ss), &len);
    if (sock != INVALID_SOCKET) {
        if (ss.ss_family == AF_INET) return {sock, SockType::SOCK_IPV4};
        else if (ss.ss_family == AF_INET6) return {sock, SockType::SOCK_IPV6};
        else if (ss.ss_family == AF_UNIX) return {sock, SockType::SOCK_UNIX};
        else CloseSocket(sock);
    }
    return Socket {SockType::INVALID};
}

void Socket::close() {
    CloseSocket(fd_);
}

bool Socket::invalid() {
    return invalid_;
}

bool Socket::setnonblocking() {
    return SetNonblockingSocket(fd_);
}

SocketBuffer::SocketBuffer(const Socket& socket) : sock_(socket), istream_(UniquePool<>(default_socket_buffer_size)), ostream_(UniquePool<>(default_socket_buffer_size)) {}

bool SocketBuffer::update() {
    // flush output buffer
    if (ostream_.position() > wpos_) {
        uint64_t remains = ostream_.position() - wpos_;
        uint64_t written = 0;
        char* dataptr = &ostream_.container()[wpos_];
        while (remains != 0) {
            uint64_t wr = send(sock_.fd_, dataptr, 1024, 0);
            if (wr == SOCKET_ERROR) {
                wpos_ += written;
                return false;
            }
            written += wr;
            remains -= wr;
        }
        return true;
    }
    // read input data
    {
        char buffer[1024] {};
        uint64_t read;
        rpos_ = istream_.position();
        while ((read = recv(sock_.fd_, buffer, 1024, 0)) != SOCKET_ERROR) {
            istream_.write(buffer, read);
        }
        istream_.position(rpos_);
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return true;
        }
        return false;
    }
}

Stream<UniquePool<>>& SocketBuffer::istream() {
    return istream_;
}

Stream<UniquePool<>>& SocketBuffer::ostream() {
    return ostream_;
}