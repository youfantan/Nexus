#include <include/net/http_server.h>
#include <include/net/http_resolver.h>
#ifdef PLATFORM_WIN32
#include <include/platform/win32/win32_io.h>
#endif

#include <unordered_map>

using namespace Nexus::IO;
using namespace Nexus::Base;
using namespace Nexus::Parallel;

uint64_t Nexus::Net::executed_sock = 0;

template<typename MUX, int N>
Nexus::Net::HttpServer<MUX, N>::HttpServer::HttpServer(Nexus::Utils::NetAddr addr , WorkGroup<N>& group) : sock_(addr.type()), iomux_(IOMultiplexer<MUX>()), group_(group) {
    if (!sock_.bind(addr)) {
        LFATAL("Error occured when bind http server to {}. Error Code: {}", addr.url(), GetLastNetworkError());
        exit(EXIT_FAILURE);
    }
    sock_.listen();
    LINFO("Http Server started on {}", addr.url());
    auto b = sock_.setnonblocking();
    iomux_.add(sock_.fd(), MUX::EVREAD);
}

template<typename MUX, int N>
void Nexus::Net::HttpServer<MUX, N>::HttpServer::loop() {
    auto evs = iomux_.poll(0);
    if (evs.is_valid()) {
        for (auto& ev : evs.reference()) {
            if (ev.handle == sock_.fd()) {
                // Server socket
                Socket client = sock_.accept();
                if (client.invalid()) {
                    client.close();
                    continue;
                }
                client.setnonblocking();
                iomux_.add(client.fd(), MUX::EVREAD | MUX::EVWRITE);
                std::shared_ptr<HttpConnection> conn = std::make_shared<HttpConnection>(client, handlers_);
                connections_.insert(std::make_pair(client.fd(), conn));
                LINFO("New Socket Connection created: {}", client.addr().url());
            } else {
                std::shared_ptr<HttpConnection> conn = connections_.at(ev.handle);
                group_.post([conn](){
                    conn->drive();
                });
            }
        }
    }
    // drive connections
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (auto it = connections_.begin(); it != connections_.end(); ) {
        std::shared_ptr<HttpConnection> conn = it->second;
        auto time_elapsed = now - conn->time_established();
        if (conn->status() == HttpConnection::FINISHED) {
            conn->cleanup();
            iomux_.remove(it->first);
            it = connections_.erase(it);
        } else if(time_elapsed > 10000) {
            LINFO("Socket Connection {} time out. Remain connections: {}", conn->get_socket().addr().url(), connections_.size());
            conn->cleanup();
            iomux_.remove(it->first);
            it = connections_.erase(it);
        } else {
            if (conn->status() == HttpConnection::EXECUTING) {
                group_.post([conn](){
                    conn->drive();
                });
            }
            ++it;
        }
    }

}
template<typename MUX, int N>
void Nexus::Net::HttpServer<MUX, N>::HttpServer::close() {
    for (auto it = connections_.begin(); it != connections_.end(); ) {
        std::shared_ptr<HttpConnection> conn = it->second;
        conn->cleanup();
        it = connections_.erase(it);
    }
}

#ifdef PLATFORM_WIN32
template class Nexus::Net::HttpServer<Win32SelectMUX, CPU_CORES - 1>;
template class Nexus::Net::HttpServer<Win32PollMUX, CPU_CORES - 1>;
#endif