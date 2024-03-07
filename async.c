/**
 * Copyright (c) 2024 Amrit Bhogal
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "async.h"

#define COMPAT53_PREFIX
#include "compat-5.3.h"

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

/*
function easyhttp.request_async(url: string, {
    method: "GET" | "POST" | "PUT" | "DELETE" | string = "GET",
    headers: { [string]: string }?,
    body: string?,
    timeout: number = 30,
    follow_redirects: boolean = true,
    max_redirects: number?,
}?, callback: function(body: string?, status_code: integer, headers: { [string]: string })): easyhttp.AsyncRequest?, string? error
*/
int easyhttp_request_async(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = lua_newuserdata(L, sizeof(struct easyhttp_AsyncRequest));
    luaL_setmetatable(L, EASYHTTP_ASYNC_REQUEST_TNAME);

    request->url = luaL_checkstring(L, 1);

    request->options = easyhttp_parse_options(L, 2);
    request->curl = curl_easy_init();
    easyhttp_set_options(request->options, &request->curl);
}

int easyhttp_async_request__gc(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    if (request->curl) {
        curl_easy_cleanup(request->curl);
    }
    luaL_unref(L, LUA_REGISTRYINDEX, request->callback_reference);
    free(request);
    return 0;
}
