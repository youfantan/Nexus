#include <include/net/http_server.h>
#include <include/net/http_resolver.h>
#ifdef PLATFORM_WIN32
#include <include/platform/win32/win32_io.h>
#endif

#include <unordered_map>

using namespace Nexus::IO;
using namespace Nexus::Base;

Nexus::Net::HttpServer::HttpServer::HttpServer(Nexus::Utils::NetAddr addr) : sock_(addr.type()){
    if (!sock_.bind(addr)) {
        throw std::runtime_error("Cannot bind server to the specific address");
    }
    sock_.listen();
}

void Nexus::Net::HttpServer::HttpServer::listen() {
#ifdef PLATFORM_WIN32
    IOMultiplexer<Win32SelectMUX> iomux;
#endif
    sock_.setnonblocking();
    iomux.add(sock_.fd(), IO_EVREAD);
    while (!flag_) {
        auto evs = iomux.poll(0);
        if (evs.is_valid()) {
            for (auto& ev : evs.reference()) {
                if (ev.handle == sock_.fd()) {
                    // Server socket
                    Socket client = sock_.accept();
                    if (client.invalid()) {
                        client.close();
                        //TODO: Log here
                        continue;
                    }
                    client.setnonblocking();
                    iomux.add(client.fd(), IO_EVREAD | IO_EVWRITE);
                    connections_.insert(std::make_pair(client.fd(), HttpConnection(client, handlers_)));
                } else {
                    HttpConnection& conn = connections_.at(ev.handle);
                    conn.drive();
                }
            }
        }
        // drive connections
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            HttpConnection conn = it->second;
            auto time_elapsed = now - conn.time_established();
            if (conn.status() == HttpConnection::FINISHED || time_elapsed > 10000) {
                conn.cleanup();
                iomux.remove(it->first);
                it = connections_.erase(it);
            } else {
                it->second.drive();
                ++it;
            }
        }
    }
}

void Nexus::Net::HttpServer::HttpServer::stop() {
    flag_ = true;
}
