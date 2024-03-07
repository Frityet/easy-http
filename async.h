// Copyright (c) 2024 Amrit Bhogal
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef EASYHTTP_ASYNC_H
#define EASYHTTP_ASYNC_H

#include "common.h"

#define EASYHTTP_ASYNC_REQUEST_TNAME "easyhttp.AsyncRequest"
struct easyhttp_AsyncRequest {
    const char *url;
    struct easyhttp_Options options;

    int callback_reference; // Lua callback function

    CURL *curl;
    struct easyhttp_Buffer *response;
    struct curl_slist *response_headers;
};

int easyhttp_request_async(lua_State *L);
int easyhttp_async_request__gc(lua_State *L);

#endif //EASYHTTP_ASYNC_H
