#include <include/net/https_server.h>
#include <include/net/http_resolver.h>
#include <include/log/logger.h>
#ifdef PLATFORM_WIN32
#include <include/platform/win32/win32_io.h>
#endif

#include <unordered_map>

using namespace Nexus::IO;
using namespace Nexus::Base;
using namespace Nexus::Parallel;

uint64_t Nexus::Net::executed_tls = 0;

template<typename MUX, int N>
Nexus::Net::HttpsServer<MUX, N>::HttpsServer(Nexus::Utils::NetAddr addr, WorkGroup<N>& group) : sock_(addr.type()), iomux_(IOMultiplexer<MUX>()), group_(group) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        LFATAL("Error occurred when creating SSL context");
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        LFATAL("Error occurred when reading SSL Certificate");
        exit(EXIT_FAILURE);
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_1_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        LFATAL("Error occurred when creating SSL Private Key");
        exit(EXIT_FAILURE);
    }
    ssl_ctx_ = ctx;
    if (!sock_.bind(addr)) {
        LFATAL("Error occured when bind http server to {}. Error Code: {}", addr.url(), GetLastNetworkError());
        exit(EXIT_FAILURE);
    }
    sock_.listen();
    sock_.setnonblocking();
    iomux_.add(sock_.fd(), IO_EVREAD);
    LINFO("Https Server started on {}", addr.url());
}
template<typename MUX, int N>
void Nexus::Net::HttpsServer<MUX, N>::HttpsServer::loop() {
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
                SSL* ssl = SSL_new(ssl_ctx_);
                SSL_set_fd(ssl, client.fd());
                client.setnonblocking();
                iomux_.add(client.fd(), IO_EVREAD | IO_EVWRITE);
                std::shared_ptr conn = std::make_shared<HttpsConnection>(client, handlers_, ssl);
                connections_.insert(std::make_pair(client.fd(), conn));
                LINFO("New TLS Connection created: {}", client.addr().url());
            } else {
                std::shared_ptr<HttpsConnection> conn = connections_.at(ev.handle);
                group_.post([conn](){
                    conn->drive();
                });
            }
        }
    }
    // drive connections
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (auto it = connections_.begin(); it != connections_.end(); ) {
        std::shared_ptr<HttpsConnection> conn = it->second;
        auto time_elapsed = now - conn->time_established();
        if (conn->status() == HttpsConnection::FINISHED) {
            conn->cleanup();
            iomux_.remove(it->first);
            it = connections_.erase(it);
        } else if(time_elapsed > 10000) {
            LINFO("TLS Connection {} time out. Remain connections: {}", conn->get_socket().addr().url(), connections_.size());
            conn->cleanup();
            iomux_.remove(it->first);
            it = connections_.erase(it);
        } else {
            if (conn->status() == HttpsConnection::EXECUTING) {
                group_.post([conn](){
                    conn->drive();
                });
            }
            ++it;
        }
    }
}

template<typename MUX, int N>
void Nexus::Net::HttpsServer<MUX, N>::HttpsServer::close() {
    for (auto it = connections_.begin(); it != connections_.end(); ) {
        std::shared_ptr<HttpsConnection> conn = it->second;
        conn->cleanup();
        it = connections_.erase(it);
    }
    EVP_cleanup();
}

#ifdef PLATFORM_WIN32
    template class Nexus::Net::HttpsServer<Win32SelectMUX, CPU_CORES - 1>;
#endif