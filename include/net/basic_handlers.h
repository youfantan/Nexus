#pragma once

#include "./http_handler.h"

class statistics_handler {
public:
    statistics_handler() = default;
    ~statistics_handler() = default;
    static http_response doGet(const get_request& gr);
    static http_response doPost(const post_request& pr);
};