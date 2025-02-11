#pragma once

#include <vector>
#include "../../io/mux.h"
#include "./win32_defs.h"

namespace Nexus::IO {
    class Win32PollMUX {
    private:
        std::vector<io_ev> fds_;
        WSAPOLLFD pfd_[EVMAX];
    public:
        inline static uint32_t EVREAD = POLLIN;
        inline static uint32_t EVWRITE = POLLOUT;
        inline static uint32_t EVEXCEPTION = POLLERR;
        bool add(SOCKET fd, io_evtyp_t evtyp) {
            fds_.push_back({fd, evtyp});
            return true;
        }

        void remove(SOCKET fd) {
            fds_.erase(std::remove_if(fds_.begin(), fds_.end(), [fd](const io_ev& f) { return f.handle == fd; }), fds_.end());
        }

        Nexus::Utils::MayFail<std::vector<io_ev>> poll(int waitms) {
            for (int i = 0; i < fds_.size(); ++i) {
                pfd_[i].fd = fds_[i].handle;
                pfd_[i].events = fds_[i].evtyp;
            }
            int r = WSAPoll(pfd_, fds_.size(), 0);
            auto ret = fds_;
            if (r != SOCKET_ERROR) {
                for (int i = 0; i < fds_.size(); ++i) {
                    ret[i].handle = pfd_[i].fd;
                    ret[i].evtyp = pfd_[i].events;
                }
                return ret;
            } else {
                return Nexus::Utils::failed;
            }
        }

        void close() {

        }
    };
    class Win32SelectMUX {
    private:
        std::vector<io_ev> fds_;
    public:
        inline static uint32_t EVREAD = 0x80000000;
        inline static uint32_t EVWRITE = 0x40000000;
        inline static uint32_t EVEXCEPTION = 0x20000000;
        Win32SelectMUX() = default;
        bool add(SOCKET fd, io_evtyp_t evtyp) {
            io_ev ie {fd, 0};
            if (MSK_SEL(evtyp, EVREAD)) MSK_SET(ie.evtyp, EVREAD);
            if (MSK_SEL(evtyp, EVWRITE)) MSK_SET(ie.evtyp, EVWRITE);
            if (MSK_SEL(evtyp, EVEXCEPTION)) MSK_SET(ie.evtyp, EVEXCEPTION);
            fds_.push_back(ie);
            return true;
        }
        void remove(SOCKET fd) {
            fds_.erase(std::remove_if(fds_.begin(), fds_.end(), [fd](const io_ev& f) { return f.handle == fd; }), fds_.end());
        }
        Nexus::Utils::MayFail<std::vector<io_ev>> poll(int waitms) {
            fd_set rset{};
            fd_set wset{};
            fd_set eset{};
            FD_ZERO(&rset);
            FD_ZERO(&wset);
            FD_ZERO(&eset);
            for(auto fd : fds_) {
                if (MSK_SEL(fd.evtyp, EVREAD)) FD_SET(fd.handle, &rset);
                if (MSK_SEL(fd.evtyp, EVWRITE)) FD_SET(fd.handle, &wset);
                if (MSK_SEL(fd.evtyp, EVEXCEPTION)) FD_SET(fd.handle, &eset);
            }
            timeval tv {
                    .tv_sec = waitms / 1000,
                    .tv_usec = (waitms % 1000) * 1000
            };
            std::vector<io_ev> r;
            if (select(0, &rset, &wset, &eset, &tv) != SOCKET_ERROR) {
                for (auto fd : fds_) {
                    io_ev ie { fd.handle, 0 };
                    if (FD_ISSET(fd.handle, &rset)) MSK_SET(ie.evtyp, EVREAD);
                    if (FD_ISSET(fd.handle, &wset)) MSK_SET(ie.evtyp, EVWRITE);
                    if (FD_ISSET(fd.handle, &eset)) MSK_SET(ie.evtyp, EVEXCEPTION);
                    if (fd.evtyp != 0) {
                        r.push_back(ie);
                    }
                }
                return r;
            } else {
                return Nexus::Utils::failed;
            }
        }
        void close() {

        }

    };

    inline static void EnableWindowsVirtualANSI() {
#ifdef LOG_ANSI_SUPPORT
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;
        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return;
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    }

}