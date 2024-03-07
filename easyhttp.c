/**
 * Copyright (c) 2024 Amrit Bhogal
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */


#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "common.h"
#include "async.h"

#define EASYHTTP_VERSION "0.1.0"

/*
function easyhttp.request(url: string, options: {
    method: "GET" | "POST" | "PUT" | "DELETE" | string = "GET",
    headers: { [string]: string }?,
    body: string?,
    timeout: number = 30,
    follow_redirects: boolean = true,
    max_redirects: number?,
}?): string? body, integer status_code, { [string]: string } headers
*/
static int easyhttp_request(lua_State *L)
{
    const char *url = luaL_checkstring(L, 1);

    if (lua_gettop(L) < 2 || lua_isnoneornil(L, 2)) {
        lua_newtable(L);
    } else {
        luaL_checktype(L, 2, LUA_TTABLE);
    }

    CURL *curl = curl_easy_init();
    struct easyhttp_Options opts = easyhttp_parse_options(L, 2);

    struct easyhttp_Buffer *buffer = easyhttp_buffer_create();
    if (!buffer) {
        return luaL_error(L, "failed to allocate memory for buffer");
    }

    easyhttp_set_options(opts, &curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, easyhttp_buffer_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return luaL_error(L, "failed to perform request: %s", curl_easy_strerror(res));
    }

    long status_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    struct curl_slist *response_headers;
    curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &response_headers);

    lua_pushlstring(L, buffer->data, buffer->size);
    lua_pushinteger(L, status_code);
    lua_newtable(L);
    for (struct curl_slist *header = response_headers; header; header = header->next) {
        const char *colon = strchr(header->data, ':');
        if (colon) {
            lua_pushlstring(L, header->data, colon - header->data);
            lua_pushstring(L, colon + 2);
            lua_settable(L, -3);
        }
    }

    curl_slist_free_all(opts.headers);
    curl_slist_free_all(response_headers);
    curl_easy_cleanup(curl);
    free(buffer);
    return 3;
}

static const struct luaL_Reg LIBRARY[] = {
    {"request", easyhttp_request},
    {"request_async", easyhttp_request_async},
    {0}
};

int luaopen_easyhttp(lua_State *L)
{
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        return luaL_error(L, "failed to initialize libcurl");
    }

    luaL_newlib(L, LIBRARY);
    lua_pushliteral(L, "_VERSION");
    lua_pushliteral(L, EASYHTTP_VERSION);
    lua_settable(L, -3);

    luaL_newmetatable(L, EASYHTTP_ASYNC_REQUEST_TNAME);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, easyhttp_async_request__gc);
    lua_settable(L, -3);
    lua_pop(L, 1);
    return 1;
}
