#pragma once

#include <unordered_map>
#include <string>
#include "../mem/memory.h"


using http_header_t = std::unordered_map<std::string, std::string>;

using http_response = struct {
    std::string response_type;
    http_header_t response_header;
    Nexus::Base::SharedPool<> response_body;
};

using get_request = struct {
    http_header_t request_handler;
};

using post_request = struct {
    http_header_t request_handler;
    Nexus::Base::SharedPool<> request_body;
};

using GetFunction = std::function<http_response(get_request&)>;
using PostFunction = std::function<http_response(post_request&)>;

struct HttpHandlerFunctionSet {
    GetFunction get;
    PostFunction post;
};

template<typename H>
concept IsHttpHandler = requires {
    { H::doGet(get_request{}) } -> std::same_as<http_response>;
    { H::doPost(post_request{{}, Nexus::Base::SharedPool<>{1024}}) } -> std::same_as<http_response>;
};