#pragma once

#include "./https_connection.h"
#include "./socket.h"
#include "../utils/netaddr.h"
#include "../io/mux.h"
#include "http_handler.h"
#include "../parallel/worker.h"

namespace Nexus::Net {
    template<typename MUX, int N>
    class HttpsServer {
    private:
        Nexus::IO::IOMultiplexer<MUX> iomux_;
        std::unordered_map<io_handle_t, HttpsConnection> connections_;
        std::unordered_map<std::string, HttpHandlerFunctionSet> handlers_;
        Socket sock_;
        bool flag_ {false};
        SSL_CTX* ssl_ctx_;
        Nexus::Parallel::WorkGroup<N>& group_;
    public:
        // Establish a socket using given addresses
        explicit HttpsServer(Nexus::Utils::NetAddr addr, Nexus::Parallel::WorkGroup<N>& group);
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