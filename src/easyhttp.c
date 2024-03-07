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
}?): (string body, integer status_code, { [string]: string } headers) | (nil, string error)
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
    const char *err = NULL;
    struct easyhttp_Options opts = easyhttp_parse_options(L, 2, &err);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }

    struct easyhttp_Buffer *buffer = easyhttp_buffer_create();
    if (!buffer) {
        lua_pushnil(L);
        lua_pushliteral(L, "failed to create buffer");
        return 2;
    }

    easyhttp_set_options(opts, curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    if (!opts.output_file) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, easyhttp_buffer_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, *opts.output_file);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        lua_pushnil(L);
        lua_pushfstring(L, "failed to perform request: %s", curl_easy_strerror(res));
        return 2;
    }

    long status_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    if (opts.output_file)
        lua_pushboolean(L, 1);
    else
        lua_pushlstring(L, buffer->data, buffer->size);
    lua_pushinteger(L, status_code);

    // Get headers
    lua_newtable(L);
    struct curl_header *header = NULL;
    while ((header = curl_easy_nextheader(curl, CURLH_HEADER, -1, header))) {
        const char *key = header->name, *value = header->value;
        lua_pushstring(L, key);
        lua_pushstring(L, value);
        lua_settable(L, -3);
    }

    curl_easy_cleanup(curl);
    free(buffer);
    return 3;
}

static const struct luaL_Reg ASYNC_METHODS[] = {
    { "is_done", easyhttp_async_request_is_done },
    { "response", easyhttp_async_request_response },
    { "cancel", easyhttp_async_request_cancel },
    {0}
};

static const struct luaL_Reg LIBRARY[] = {
    { "request", easyhttp_request },
    { "async_request", easyhttp_async_request },
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

    lua_pushliteral(L, "__index");
    lua_newtable(L);
    luaL_setfuncs(L, ASYNC_METHODS, 0);
    lua_settable(L, -3);

    lua_pop(L, 1);
    return 1;
}
