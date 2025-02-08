#pragma once
#include <chrono>
#include <ranges>
#include "./socket.h"
#include "./http_resolver.h"
#include "../utils/netaddr.h"
#include "../io/resource_locator.h"
#include "http_handler.h"

#ifdef PLATFORM_WIN32
#include <include/platform/win32/win32_io.h>
#endif

namespace Nexus::Net {
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
        Nexus::Base::SharedPool<> buffer_;
        Nexus::Base::Stream<decltype(buffer_)> stream_;
        status_t status_ {READ};
        HttpResolver resolver_;
        http_header_t headers_;
        uint64_t content_length_ {0};
        Nexus::Base::SharedPool<> response_;
        Nexus::Base::Stream<decltype(response_)> resp_stream_;
    public:
        HttpConnection(Socket sock, std::unordered_map<std::string, HttpHandlerFunctionSet>& handlers) : sock_(sock), buffer_(1024), stream_(buffer_), resolver_(buffer_),
                                               response_(1024), resp_stream_(response_), handlers_(handlers) {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            established_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        }
        void drive() {
            switch (status_) {
                case READ: {
                    int r;
                    char buf[1024];
                    while ((r = recv(sock_.fd(), buf, 1024, 0)) > 0) {
                        stream_.write(buf, r);
                    }
                    if (r == 0 || (WSAGetLastError() != WSAEWOULDBLOCK)) {
                        // abnormal status, cleanup resources. TODO: log it out
                        cleanup();
                        break;
                    }
                    if (resolver_.header_ended()) {
                        auto method = resolver_.resolve_method();
                        headers_ = resolver_.resolve_headers();
                        if (method == http_method::GET) {
                            status_ = EXECUTING;
                            return;
                        } else if (method == http_method::POST) {
                            if (!headers_.contains("Content-Length")) {
                                response("400 Bad Request", {});
                                break;
                            }
                            try {
                                content_length_ = std::stoul(headers_["Content-Length"]);
                                uint64_t curr = buffer_.limit() - resolver_.resolve_header_end();
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
                    if (resolver_.resolve_method() == http_method::GET) {
                        auto path = resolver_.resolve_path();
                        if (handlers_.contains(path)) {
                            HttpHandlerFunctionSet& fs = handlers_.at(path);
                            get_request gr { headers_ };
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
                            if (r.is_valid()) {
                                response("200 OK", {
                                        {"Content-Type", r.reference().mime}
                                }, r.reference().data);
                            } else {
                                Nexus::Base::SharedPool<> resp(1024);
                                std::string str = "<html><body><h1>404 Not Found</h1><p>Server: Nexus@BetaV1</p></body></html>";
                                resp.write(str.data(), str.size());
                                response("404 Not Found", {
                                        {"Content-Type", "text/html"}
                                }, resp);
                            }
                        }
                    } else if (resolver_.resolve_method() == http_method::POST) {
                        Nexus::Base::SharedPool<> body(1024);
                        body.write(&buffer_[resolver_.resolve_header_end() + 1], content_length_);
                        auto path = resolver_.resolve_path();
                        if (handlers_.contains(path)) {
                            HttpHandlerFunctionSet &fs = handlers_.at(path);
                            post_request pr{headers_, body};
                            http_response resp = fs.post(pr);
                            if (resp.response_body.limit() == 0) {
                                response(resp.response_type, resp.response_header);
                            } else {
                                response(resp.response_type, resp.response_header, resp.response_body);
                            }
                        } else {
                            Nexus::Base::SharedPool<> resp(1024);
                            std::string str = "Handler Not Found | Nexus@BetaV1";
                            resp.write(str.data(), str.size());
                            response("404 Not Found", {
                                    {"Content-Type", "text/plain"}
                            }, resp);
                        }
                    }
                    break;
                }
                case RESPONSE: {
                    int r;
                    do {
                        auto buf = resp_stream_.read(1024);
                        // buf must be valid
                        r = send(sock_.fd(), buf.reference().ptr(), static_cast<int>(buf.reference().size()), 0);
                    } while (resp_stream_.flag() != Nexus::Base::SharedPool<>::flag_t::eof && r > 0);
                    if (WSAGetLastError() != WSAEWOULDBLOCK) {
                        cleanup();
                    }
                    break;
                }
                case FINISHED:
                    break;
            }
        }

        status_t status() {
            return status_;
        }

        void response(const std::string& status, const http_header_t& headers, Nexus::Base::SharedPool<>& content) {
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
            response_.write(&content[0], content.limit());
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
                stream_.close();
                sock_.close();
            }
        }
        uint64_t time_established() {
            return established_time_;
        }
    };


    

}