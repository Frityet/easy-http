// Copyright (c) 2024 Amrit Bhogal
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef EASYHTTP_COMMON_H
#define EASYHTTP_COMMON_H

#define COMPAT53_PREFIX
#include "extern/compat-5.3.h"

#if __STDC_VERSION__ < 201112L || defined(__STDC_NO_THREADS__)
#   include "extern/tinycthread.h"
#else
    // If the compiler claims to support C11, you might still need to include threads.h for C11 thread support
#   include <threads.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>

struct easyhttp_Buffer {
    size_t size;
    char data[];
};

struct easyhttp_Options {
    const char *method, *body;
    bool follow_redirects;
    int timeout, max_redirects;
    FILE **output_file;
    struct curl_slist *headers;
};

static const struct easyhttp_Options EASYHTTP_DEFAULT_OPTIONS = {
    .method = "GET",
    .body = NULL,
    .timeout = 0,
    .follow_redirects = false,
    .max_redirects = -1,
    .headers = NULL
};

static struct easyhttp_Options easyhttp_parse_options(lua_State *L, int idx, const char **error)
{
    struct easyhttp_Options options = EASYHTTP_DEFAULT_OPTIONS;

    lua_getfield(L, idx, "method");
    if (!lua_isnil(L, -1)) {
        options.method = luaL_checkstring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "body");
    if (!lua_isnil(L, -1)) {
        options.body = luaL_checkstring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "timeout");
    if (!lua_isnil(L, -1)) {
        options.timeout = luaL_checkinteger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "follow_redirects");
    if (!lua_isnil(L, -1)) {
        options.follow_redirects = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "max_redirects");
    if (!lua_isnil(L, -1)) {
        options.max_redirects = luaL_checkinteger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "output_file");
    if (!lua_isnil(L, -1)) {
        options.output_file = luaL_checkudata(L, -1, "FILE*");
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "headers");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);

        size_t alloc_siz = 0;
        char *buf = malloc(1);
        if (!buf) {
            *error = "failed to allocate memory for headers";
            return options;
        }
        while (lua_next(L, -2)) {
            const char *key = luaL_checkstring(L, -2);
            const char *value = luaL_checkstring(L, -1);
            size_t key_len = strlen(key), value_len = strlen(value);
            if (key_len + value_len + 3 > alloc_siz) {
                alloc_siz = key_len + value_len + 3;
                void *tmp = realloc(buf, alloc_siz);
                if (!tmp) {
                    *error = "failed to resize memory for headers";
                    return options;
                }
                buf = tmp;
            }

            snprintf(buf, alloc_siz, "%s: %s", key, value);
            options.headers = curl_slist_append(options.headers, buf);
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    *error = 0;
    return options;
}

static inline void easyhttp_set_options(struct easyhttp_Options options, CURL *curl)
{
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, options.method);
    if (options.body)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, options.timeout);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, options.follow_redirects);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, options.max_redirects);
    if (options.headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, options.headers);
}

#pragma region Buffer
static struct easyhttp_Buffer *easyhttp_buffer_create(void)
{
    struct easyhttp_Buffer *buffer = calloc(1, sizeof(struct easyhttp_Buffer) + 1);
    if (!buffer) {
        return NULL;
    }
    return buffer;
}

static struct easyhttp_Buffer *easyhttp_buffer_append(struct easyhttp_Buffer *buffer, size_t size, const char data[static size])
{
    size_t new_size = buffer->size + size;
    void *tmp = realloc(buffer, sizeof(struct easyhttp_Buffer) + new_size + 1);
    if (!tmp) {
        return NULL;
    }
    buffer = tmp;
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size = new_size;
    buffer->data[buffer->size] = '\0';
    return buffer;
}

static int easyhttp_buffer_write(void *data, size_t size, size_t nmemb, void *userp)
{
    struct easyhttp_Buffer **buffer = userp;
    size_t new_size = (*buffer)->size + size * nmemb;

    *buffer = easyhttp_buffer_append(*buffer, size * nmemb, data);
    if (!*buffer) {
        return 0;
    }

    return size * nmemb;
}
#pragma endregion

#endif //EASYHTTP_COMMON_H
