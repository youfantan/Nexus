#pragma once

#include <vector>
#include "../../io/mux.h"
#include "./win32_defs.h"

namespace Nexus::IO {
    class Win32SelectMUX {
    private:
        std::vector<io_ev> fds_;
    private:
    public:
        Win32SelectMUX() = default;
        bool add(SOCKET fd, io_evtyp_t evtyp) {
            io_ev ie {fd, 0};
            if (MSK_SEL(evtyp, IO_EVREAD)) MSK_SET(ie.evtyp, IO_EVREAD);
            if (MSK_SEL(evtyp, IO_EVWRITE)) MSK_SET(ie.evtyp, IO_EVWRITE);
            if (MSK_SEL(evtyp, IO_EVEXCETION)) MSK_SET(ie.evtyp, IO_EVEXCETION);
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
                if (MSK_SEL(fd.evtyp, IO_EVREAD)) FD_SET(fd.handle, &rset);
                if (MSK_SEL(fd.evtyp, IO_EVWRITE)) FD_SET(fd.handle, &wset);
                if (MSK_SEL(fd.evtyp, IO_EVEXCETION)) FD_SET(fd.handle, &eset);
            }
            timeval tv {
                    .tv_sec = waitms / 1000,
                    .tv_usec = (waitms % 1000) * 1000
            };
            std::vector<io_ev> r;
            if (select(0, &rset, &wset, &eset, &tv) != SOCKET_ERROR) {
                for (auto fd : fds_) {
                    io_ev ie { fd.handle, 0 };
                    if (FD_ISSET(fd.handle, &rset)) MSK_SET(ie.evtyp, IO_EVREAD);
                    if (FD_ISSET(fd.handle, &wset)) MSK_SET(ie.evtyp, IO_EVWRITE);
                    if (FD_ISSET(fd.handle, &eset)) MSK_SET(ie.evtyp, IO_EVEXCETION);
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