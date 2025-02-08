#pragma once

#include "../mem/memory.h"
#include "./http_handler.h"
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
        uint64_t last_ {0};
        std::vector<uint64_t> marks_;
        Nexus::Base::SharedPool<> pool_;
        bool cached_ {false};
        bool find_end(const char* str, uint64_t size) {
            for (int i = 0; i < size; ++i) {
                if (str[i] == '\r') {
                    marks_.push_back(last_ + i - 1);
                    if (str[i + 1] == '\n' && str[i + 2] == '\r' && str[i + 3] == '\n') {
                        cached_ = true;
                        return true;
                    }
                }
            }
            return false;
        }
        std::pair<std::string, std::string> resolve_header(uint64_t beg, uint64_t end) {
            char* ptr = &pool_[beg];
            uint64_t split = 0;
            bool space = false;
            for (int i = 0; i < end - beg; ++i) {
                if (ptr[i] == ':') {
                    split = i;
                    if (ptr[i + 1] == ' ') {
                        space = true;
                    }
                }
            }
            std::string key(ptr, split);
            auto kb = space ? split + 2 : split + 1;
            std::string value(ptr + kb, end - beg - kb + 1);
            return {key, value};
        }
    public:
        explicit HttpResolver(Nexus::Base::SharedPool<>& pool) : pool_(pool) {}
        bool header_ended() {
            if (cached_) return true;
            using namespace Nexus::Base;
            if (find_end(&pool_[last_], pool_.limit() - last_)) return true;
            last_ = pool_.limit();
            return false;
        }
        http_header_t& resolve_headers() {
            uint64_t beg = marks_[0] + 3    ;
            for (int i = 1; i < marks_.size(); ++i) {
                auto header = resolve_header(beg, marks_[i]);
                headers_.emplace(header.first, header.second);
                beg = marks_[i] + 3;
            }
            return headers_;
        }
        http_method resolve_method() {
            if (pool_[0] == 'G' && pool_[1] == 'E' && pool_[2] == 'T') {
                return http_method::GET;
            } else if (pool_[0] == 'P' && pool_[1] == 'O' && pool_[2] == 'S' && pool_[3] == 'T') {
                return http_method::POST;
            }
            return http_method::UNSUPPORTED;
        }
        std::string resolve_path() {
            char* beg = &pool_[0];
            uint64_t path_beg = 0;
            uint64_t path_end = 0;
            for (int i = 0; i < marks_[0] + 1; ++i) {
                if (path_beg == 0) {
                    if (beg[i] == '/') {
                        path_beg = i;
                    }
                } else {
                    if (beg[i] == ' ') {
                        path_end = i;
                    }
                }
            }
            if (path_beg == 0 || path_end == 0) {
                return {};
            }
            return {beg, path_beg, path_end - path_beg};
        }
        uint64_t resolve_header_end() {
            return marks_.back() + 4;
        }
    };
}