#pragma once

#include "../mem/memory.h"
#include "./http_handler.h"
#include "../../thirdparty/picohttpparser/picohttpparser.h"
#include <iostream>
#include <unordered_map>
namespace Nexus::Net {

    enum class http_method {
        GET,
        POST,
        UNSUPPORTED
    };

    class HttpResolver {
    private:
        http_header_t headers_;
        std::string method_;
        std::string path_;
        uint64_t request_len_;
        Nexus::Base::SharedPool<Nexus::Base::AlignedHeapAllocator<4>> buffer_;
        bool cached_ {false};
        bool resolve(const char* str, uint64_t size) {
            const char *method, *path;
            size_t method_len, path_len;
            int minor_version;
            struct phr_header headers[64];
            size_t num_headers = sizeof(headers) / sizeof(headers[0]);
            int ret = phr_parse_request(
                    str, size,
                    &method, &method_len,
                    &path, &path_len,
                    reinterpret_cast<int *>(&minor_version),
                    headers, &num_headers, 0
            );
            if (ret > 0) {
                for (int i = 0; i < num_headers; ++i) {
                    cached_ = true;
                    headers_.emplace(std::string(headers[i].name, headers[i].name_len), std::string(headers[i].value, headers[i].value_len));
                }
                method_ = std::string(method, method_len);
                path_ = std::string(path, path_len);
                request_len_ = ret;
                return true;
            } else {
                return false;
            }
        }
    public:
        explicit HttpResolver(Nexus::Base::SharedPool<Nexus::Base::AlignedHeapAllocator<4>>& pool) : buffer_(pool) {}
        bool header_ended() {
            using namespace Nexus::Base;
            return (resolve(&buffer_[0], buffer_.limit()));
        }
        http_header_t& resolve_headers() {
            return headers_;
        }
        http_method resolve_method() {
            if (method_ == "GET") {
                return http_method::GET;
            } else if (method_ == "POST") {
                return http_method::POST;
            }
            return http_method::UNSUPPORTED;
        }
        std::string resolve_path() {
            return path_;
        }

        uint64_t resolve_header_end() {
            return request_len_;
        }
    };
}