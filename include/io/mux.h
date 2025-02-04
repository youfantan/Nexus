#pragma once

#include <cstdint>
#include <concepts>
#include <vector>
#include "../utils/check.h"

#ifdef PLATFORM_WIN32
#include "include/platform/win32/win32_defs.h"
#endif

namespace Nexus::IO {
#define IO_EVREAD 0x80000000
#define IO_EVWRITE 0x40000000
#define IO_EVEXCETION 0x20000000
#define MSK_SEL(dat, mask) (((dat) & (mask)) == (mask))
#define MSK_SET(dat, mask) ((dat) |= (mask))
#define EVMAX 1024

    using io_evtyp_t = uint32_t;

    struct io_ev {
        io_handle_t handle;
        io_evtyp_t evtyp;
    };

    template<typename MUX>
    concept IsIOMUX = requires(MUX mux) {
        MUX();
        { mux.poll() } -> std::same_as<Nexus::Utils::MayFail<std::vector<io_ev>>>;
        { mux.add(HANDLE_MAX, UINT32_MAX) } -> std::same_as<bool>;
        { mux.remove(HANDLE_MAX) };
        mux.close(); // IOMUX must take appropriate measures to avoid double-free errors
    };

    template<typename MUX> requires IsIOMUX<MUX>
    class IOMultiplexer {
    private:
        MUX mux_;
    public:
        IOMultiplexer() : mux_(MUX()) {}
        Nexus::Utils::MayFail<std::vector<io_ev>> poll() {
            return mux_.poll();
        }
        bool add(io_handle_t handle, io_evtyp_t evtyp) {
            return mux_.add(handle, evtyp);
        }
        void remove(io_handle_t handle) {
            return mux_.remove(handle);
        }
        void close() {
            mux_.close();
        }
        ~IOMultiplexer() {
            mux_.close();
        }
    };


}