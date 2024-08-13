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

#define EASYHTTP_VERSION "0.1.2"

struct WriteArgs {
    struct easyhttp_Buffer **buffer;
    FILE *file;
    struct easyhttp_Options options;
    lua_State *L;
};

struct ProgressArgs {
    LuaReference_t on_progress;
    lua_State *L;
};

static int write_callback(void *data, size_t size, size_t nmemb, void *userp)
{
    struct WriteArgs *args = (struct WriteArgs *)userp;
    size_t fsiz = size * nmemb;

    char *modified_output = NULL;
    if (args->options.on_data != LUA_NOREF) {
        lua_rawgeti(args->L, LUA_REGISTRYINDEX, args->options.on_data);
        lua_pushlstring(args->L, data, fsiz);
        lua_pushinteger(args->L, size);
        lua_pushinteger(args->L, nmemb);
        lua_call(args->L, 3, 1);
        switch (lua_type(args->L, -1)) {
            case LUA_TSTRING: {
                size_t slen = 0;
                const char *str = lua_tolstring(args->L, -1, &slen);
                modified_output = string_duplicate_n(str, slen);
                data = modified_output;
                size = 1;
                nmemb = slen;
                break;
            }
            case LUA_TBOOLEAN: {
                if (lua_toboolean(args->L, -1) == false)
                    return 0;
                break;
            }
            default: break;
        }
    }

    if (args->options.output_file) {
        fwrite(data, size, nmemb, args->file);
    } else {
        easyhttp_buffer_write(data, size, nmemb, args->buffer);
    }

    free(modified_output);

    return fsiz;
}

static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    struct ProgressArgs *args = (struct ProgressArgs *)clientp;

    int retc = 0;
    if (args->on_progress != LUA_NOREF) {
        lua_rawgeti(args->L, LUA_REGISTRYINDEX, args->on_progress);
        lua_pushnumber(args->L, dltotal);
        lua_pushnumber(args->L, dlnow);
        lua_pushnumber(args->L, ultotal);
        lua_pushnumber(args->L, ulnow);
        lua_call(args->L, 4, 1);

        if (lua_isinteger(args->L, -1)) {
            retc = lua_tointeger(args->L, -1);
        }
    }

    return retc;
}

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
    struct easyhttp_Options opts = easyhttp_options_parse(L, 2, &err);
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

    easyhttp_options_set(opts, curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct WriteArgs args = {
        .buffer = &buffer,
        .file = opts.output_file ? *opts.output_file : NULL,
        .options = opts,
        .L = L
    };
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &args);

    struct ProgressArgs progress_args = {
        .on_progress = opts.on_progress,
        .L = L
    };
    if (opts.on_progress != LUA_NOREF) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress_args);
    }

    struct easyhttp_Headers *headers = easyhttp_headers_create();
    if (!headers) {
        lua_pushnil(L);
        lua_pushliteral(L, "failed to create result headers");
        return 2;
    }
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, easyhttp_headers_write);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

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
        lua_pushlstring(L, buffer->data, buffer->length);
    lua_pushinteger(L, status_code);

    // Get headers
    lua_newtable(L);
    for (size_t i = 0; i < headers->length; i++) {
        lua_pushstring(L, headers->headers[i].key);
        lua_pushstring(L, headers->headers[i].value);
        lua_settable(L, -3);
    }

    easyhttp_options_free(&opts);
    curl_easy_cleanup(curl);
    free(buffer);
    return 3;
}

static const struct luaL_Reg ASYNC_METHODS[] = {
    { "is_done", easyhttp_async_request_is_done },
    { "response", easyhttp_async_request_response },
    { "cancel", easyhttp_async_request_cancel },
    { "data", easyhttp_async_request_data },
    { "progress", easyhttp_async_request_progress },
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
