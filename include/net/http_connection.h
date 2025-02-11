#pragma once
#include <chrono>
#include <ranges>
#include "./socket.h"
#include "./http_resolver.h"
#include "../utils/netaddr.h"
#include "../io/resource_locator.h"
#include "http_handler.h"
#include "../log/logger.h"

#ifdef PLATFORM_WIN32
#include <include/platform/win32/win32_io.h>
#endif

namespace Nexus::Net {
    extern uint64_t executed_sock;
    class HttpConnection {
    public:
        using status_t = enum {
            READ,
            EXECUTING,
            RESPONSE,
            FINISHED
        };
    private:
        std::unordered_map<std::string, HttpHandlerFunctionSet>& handlers_;
        Socket sock_;
        uint64_t established_time_;
        Nexus::Base::SharedPool<Nexus::Base::AlignedHeapAllocator<4>> request_;
        Nexus::Base::Stream<decltype(request_)> req_stream_;
        Nexus::Base::SharedPool<> response_;
        Nexus::Base::Stream<decltype(response_)> resp_stream_;
        status_t status_ {READ};
        HttpResolver resolver_;
        uint64_t content_length_ {0};
        std::mutex mtx_;
    public:
        HttpConnection(const Socket& sock, std::unordered_map<std::string, HttpHandlerFunctionSet>& handlers) : sock_(sock), request_(1024), req_stream_(request_), resolver_(request_),
                                                                                                         response_(1024), resp_stream_(response_), handlers_(handlers), mtx_(std::mutex{}) {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            established_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        }

        void drive() {
            mtx_.lock();
            switch (status_) {
                case READ: {
                    int r;
                    char buf[1024];
                    while ((r = recv(sock_.fd(), buf, 1024, 0)) > 0) {
                        req_stream_.write(buf, r);
                    }
                    if (r == 0 || (GetLastNetworkError() != WSAEWOULDBLOCK)) {
                        LWARN("Socket read error, closing Socket connection: {}. Errno: {} | {}", sock_.addr().url(), GetLastNetworkError(), GetLastSystemError());
                        cleanup();
                        break;
                    }
                    if (resolver_.header_ended()) {
                        auto method = resolver_.resolve_method();
                        resolver_.resolve_headers() ;
                        if (method == http_method::GET) {
                            status_ = EXECUTING;
                            break;
                        } else if (method == http_method::POST) {
                            if (!resolver_.resolve_headers().contains("Content-Length")) {
                                response("400 Bad Request", {});
                                break;
                            }
                            try {
                                content_length_ = std::stoul(resolver_.resolve_headers()["Content-Length"]);
                                uint64_t curr = request_.limit() - resolver_.resolve_header_end();
                                if (curr >= content_length_) {
                                    status_ = EXECUTING;
                                }
                            } catch (std::exception& e) {
                                response("400 Bad Request", {});
                                break;
                            }
                        } else {
                            response("405 Method Not Allowed", {});
                            break;
                        }
                    }
                    break;
                }
                case EXECUTING: {
                    executed_sock++;
                    if (resolver_.resolve_method() == http_method::GET) {
                        auto path = resolver_.resolve_path();
                        LINFO("New Http Request: GET {} from {}", path, sock_.addr().url());
                        if (handlers_.contains(path)) {
                            HttpHandlerFunctionSet& fs = handlers_.at(path);
                            get_request gr { resolver_.resolve_headers() };
                            http_response resp = fs.get(gr);
                            if (resp.response_body.limit() == 0) {
                                response(resp.response_type, resp.response_header);
                            } else {
                                response(resp.response_type, resp.response_header, resp.response_body);
                            }
                        } else {
                            if (path == "/") {
                                path.append("index.html");
                            }
                            auto r = Nexus::IO::ResourceLocator::LocateResource(path);
                            if (r.valid) {
                                response("200 OK", {
                                        {"Content-Type", r.mime}
                                }, r.data);
                            } else {
                                response("404 Not Found", {
                                        {"Content-Type", "text/html"}
                                }, Nexus::Base::FixedPool(get_not_found_resp.data(), get_not_found_resp.size()));
                            }
                        }
                    } else if (resolver_.resolve_method() == http_method::POST) {
                        char* body = reinterpret_cast<char*>(malloc(content_length_));
                        memcpy(body, &request_[resolver_.resolve_header_end() + 1], content_length_);
                        auto path = resolver_.resolve_path();
                        LINFO("New Http Request: POST /{} from {}", path, sock_.addr().url());
                        if (handlers_.contains(path)) {
                            HttpHandlerFunctionSet &fs = handlers_.at(path);
                            post_request pr{resolver_.resolve_headers(), Nexus::Base::FixedPool<true, Nexus::Base::HeapAllocator>(body, content_length_)};
                            http_response resp = fs.post(pr);
                            if (resp.response_body.limit() == 0) {
                                response(resp.response_type, resp.response_header);
                            } else {
                                response<true>(resp.response_type, resp.response_header, resp.response_body);
                            }
                        } else {
                            response("404 Not Found", {
                                    {"Content-Type", "text/plain"}
                            }, Nexus::Base::FixedPool(post_not_found_resp.data(), post_not_found_resp.size()));
                        }
                    }
                    break;
                }
                case RESPONSE: {
                    int r;
                    do {
                        auto buf = resp_stream_.read(1024);
                        r = send(sock_.fd(), buf.reference().ptr(), static_cast<int>(buf.reference().size()), 0);
                    } while (resp_stream_.flag() != Nexus::Base::SharedPool<>::flag_t::eof && r > 0);
                    if (r < 0) {
                        if (WSAGetLastError() != WSAEWOULDBLOCK) {
                            LWARN("Socket write error, closing Socket connection: {}. Errno: {} | {}", sock_.addr().url(), GetLastNetworkError(), GetLastSystemError());
                            cleanup();
                        }
                    } else if (r == 0) {
                        cleanup();
                    }
                    break;
                }
                case FINISHED:
                    break;
            }
            mtx_.unlock();
        }

        status_t status() {
            return status_;
        }
        template<bool auto_free = false>
        void response(const std::string& status, const http_header_t& headers, const Nexus::Base::FixedPool<auto_free>& content) {
            status_ = RESPONSE;
            std::stringstream ss;
            ss << "HTTP/1.1 ";
            ss << status << "\r\n";
            const_cast<http_header_t&>(headers).emplace("Content-Length", std::to_string(content.limit()));
            std::ranges::for_each(headers, [&ss](const auto& pair) {
                ss << pair.first << ": " << pair.second << "\r\n";
            });
            auto prefix = ss.str();
            response_.write(prefix.data(), prefix.size());
            response_.write("\r\n", 2);
            response_.write(content.ptr(), content.limit());
            resp_stream_.position(0);
        }

        void response(const std::string& status, const http_header_t& headers) {
            status_ = RESPONSE;
            std::stringstream ss;
            ss << "HTTP/1.1 ";
            ss << status << "\r\n";
            std::ranges::for_each(headers, [&ss](const auto& pair) {
                ss << pair.first << ": " << pair.second << "\r\n";
            });
            auto prefix = ss.str();
            response_.write(prefix.data(), prefix.size());
            response_.write("\r\n", 2);
            resp_stream_.position(0);
        }

        void cleanup() {
            if (status_ != FINISHED) {
                status_ = FINISHED;
                sock_.close();
            }
        }

        uint64_t time_established() {
            return established_time_;
        }

        Socket& get_socket() {
            return sock_;
        }
    };


    

}