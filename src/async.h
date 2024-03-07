// Copyright (c) 2024 Amrit Bhogal
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef EASYHTTP_ASYNC_H
#define EASYHTTP_ASYNC_H

#include "common.h"


#define EASYHTTP_ASYNC_REQUEST_TNAME "easyhttp.AsyncRequest"
struct easyhttp_AsyncRequest {
    struct {
        const char *url;
        struct easyhttp_Options options;
        struct easyhttp_Buffer *response;

        long response_code;
        struct easyhttp_Headers *headers;
    } request;

    const char *error;
    bool cancelled, done;
    mtx_t mutex;
    thrd_t thread;
};

int easyhttp_async_request(lua_State *L);
int easyhttp_async_request_is_done(lua_State *L);
int easyhttp_async_request_cancel(lua_State *L);
int easyhttp_async_request_response(lua_State *L);
int easyhttp_async_request__gc(lua_State *L);

#endif //EASYHTTP_ASYNC_H
