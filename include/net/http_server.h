#pragma once

#include "./socket.h"
#include "../utils/netaddr.h"
#include "http_connection.h"
#include "http_handler.h"

namespace Nexus::Net {
    class HttpServer {
    private:
        std::unordered_map<io_handle_t, HttpConnection> connections_;
        std::unordered_map<std::string, HttpHandlerFunctionSet> handlers_;
        Socket sock_;
        bool flag_ {false};
    public:
        // Establish a socket using given addresses
        explicit HttpServer(Nexus::Utils::NetAddr addr);

        template<typename H> requires IsHttpHandler<H>
        void add_handler(const std::string& path) {
            HttpHandlerFunctionSet fs {H::doGet, H::doPost};
            handlers_[path] = fs;
        }
        // Start the accept loops, subsequent operations are completed by callback
        void listen();
        // Stop the server
        void stop();
    };
}