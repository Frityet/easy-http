// Copyright (c) 2024 Amrit Bhogal
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef EASYHTTP_MULTI_H
#define EASYHTTP_MULTI_H

#include "common.h"

struct easyhttp_MultiRequestOptions {
    struct easyhttp_Options base;

    LuaReference_t on_finish; //function(response: string, code: number, headers: { [string]: string })
    LuaReference_t on_error; //function(error: string)
};

#define EASYHTTP_MULTI_REQUEST_TNAME "easyhttp.MultiRequest"
struct easyhttp_MultiRequest {
    CURLM *multi_handle;

    size_t count;
    CURL *(*handles)[];
    struct easyhttp_MultiRequestOptions (*options)[];
    const char *(*urls)[];
};

static struct easyhttp_MultiRequestOptions easyhttp_multi_request_options_parse(lua_State *L, int idx, const char **error)
{
    struct easyhttp_MultiRequestOptions options = {
        .base = easyhttp_options_parse(L, idx, error),
        .on_finish = LUA_NOREF,
        .on_error = LUA_NOREF
    };
    if (*error) return options;

    options_getfield(on_finish, easyhttp_lua_checkfunction);
    options_getfield(on_error, easyhttp_lua_checkfunction);

    return options;
}

/*
function easyhttp.multi_request(options: { [string] : easyhttp.MultiRequestOptions }): easyhttp.MultiRequest
*/
int easyhttp_multi_request(lua_State *L);
/*
function easyhttp.MultiRequest:completed_requests(): integer
*/
int easyhttp_multi_request_completed_requests(lua_State *L);
/*
function easyhttp.MultiRequest:__len(): integer
*/
int easyhttp_multi_request__len(lua_State *L);
int easyhttp_multi_request__gc(lua_State *L);

#endif
