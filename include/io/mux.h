#pragma once

#include <cstdint>
#include <concepts>
#include <vector>
#include "../utils/check.h"

#ifdef PLATFORM_WIN32
#include "include/platform/win32/win32_defs.h"
#endif

namespace Nexus::IO {
#define MSK_SEL(dat, mask) (((dat) & (mask)) == (mask))
#define MSK_SET(dat, mask) ((dat) |= (mask))
#define EVMAX 1024

    using io_evtyp_t = uint32_t;

    struct io_ev {
        io_handle_t handle;
        io_evtyp_t evtyp;
    };

    template<typename MUX>
    concept IsMultiplexer = requires(MUX mux) {
        MUX();
        { MUX::EVREAD } -> std::same_as<uint32_t&>;
        { MUX::EVWRITE } -> std::same_as<uint32_t&>;
        { MUX::EVEXCEPTION } -> std::same_as<uint32_t&>;
        { mux.poll(-INT32_MAX) } -> std::same_as<Nexus::Utils::MayFail<std::vector<io_ev>>>;
        { mux.add(HANDLE_MAX, UINT32_MAX) } -> std::same_as<bool>;
        { mux.remove(HANDLE_MAX) };
        mux.close(); // IOMUX must take appropriate measures to avoid double-free errors
    };

    template<typename MUX> requires IsMultiplexer<MUX>
    class IOMultiplexer {
    private:
        MUX mux_;
    public:
        IOMultiplexer() : mux_(MUX()) {}
        Nexus::Utils::MayFail<std::vector<io_ev>> poll(int waitms) {
            return mux_.poll(waitms);
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