#pragma once

#include "./socket.h"
#include "../utils/netaddr.h"
#include "../io/mux.h"
#include "http_connection.h"
#include "http_handler.h"
#include "../parallel/worker.h"

namespace Nexus::Net {
    template<typename MUX, int N>
    class HttpServer {
    private:
        Nexus::IO::IOMultiplexer<MUX> iomux_;
        std::unordered_map<io_handle_t, std::shared_ptr<HttpConnection>> connections_;
        std::unordered_map<std::string, HttpHandlerFunctionSet> handlers_;
        Socket sock_;
        bool flag_ {false};
        Nexus::Parallel::WorkGroup<N>& group_;
    public:
        // Establish a socket using given addresses
        explicit HttpServer(Nexus::Utils::NetAddr addr, Nexus::Parallel::WorkGroup<N>& group);
        // Add http handler with given path
        template<typename H> requires IsHttpHandler<H>
        void add_handler(const std::string& path) {
            HttpHandlerFunctionSet fs {H::doGet, H::doPost};
            handlers_[path] = fs;
        }
        // Start the accept loops, subsequent operations are completed by callback
        void loop();
        // Stop the server
        void close();
    };
}