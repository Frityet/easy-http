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
#   include <threads.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>

struct easyhttp_Buffer {
    size_t cap, length;
    char data[];
};

struct easyhttp_Options {
    const char *method, *body;
    bool follow_redirects;
    int timeout, max_redirects;
    FILE **output_file;
    struct curl_slist *headers;
};

struct easyhttp_Header {
    char *key, *value;
};
struct easyhttp_Headers {
    size_t length;
    struct easyhttp_Header headers[];
};

static inline char *string_duplicate_n(const char *str, size_t len)
{
    char *dup = malloc(len + 1);
    if (!dup) return NULL;
    memcpy(dup, str, len);
    dup[len] = '\0';
    return dup;
}

static inline char *string_duplicate(const char *str)
{
    return string_duplicate_n(str, strlen(str));
}

#pragma region Headers

static inline struct easyhttp_Headers *easyhttp_headers_create()
{ return calloc(1, sizeof(struct easyhttp_Headers)); }

static struct easyhttp_Headers *easyhttp_headers_append(struct easyhttp_Headers *headers, const char *key, const char *value)
{
    size_t new_len = headers->length + 1;
    void *tmp = realloc(headers, sizeof(struct easyhttp_Headers) + new_len * sizeof(struct easyhttp_Header));
    if (!tmp) {
        return NULL;
    }
    headers = tmp;
    headers->length = new_len;

    headers->headers[new_len - 1] = (struct easyhttp_Header) {
        .key = string_duplicate(key),
        .value = string_duplicate(value)
    };

    return headers;
}

static size_t easyhttp_headers_write(char *buf, size_t size, size_t nmemb, void *userp)
{
    struct easyhttp_Headers **headers = userp;
    char *header = string_duplicate_n(buf, size * nmemb);
    if (!header)
        return 0;


    char *colon = strchr(header, ':');
    if (!colon) {
        free(header);
        return size * nmemb;
    }

    *colon = '\0';
    char *value = colon + 1;
    while (*value == ' ' || *value == '\t') value++;
    //set last char to null to ignore trailing newlines
    char *end = value + strlen(value) - 1;
    while (*end == '\n' || *end == '\r') *end-- = '\0';

    *headers = easyhttp_headers_append(*headers, header, value);
    if (!*headers || !(*headers)->headers[(*headers)->length - 1].key || !(*headers)->headers[(*headers)->length - 1].value) {
        free(header);
        return 0;
    }
    free(header);
    return size * nmemb;
}

static void easyhttp_headers_free(struct easyhttp_Headers **headers)
{
    if (!headers || !*headers) return;
    for (size_t i = 0; i < (*headers)->length; i++) {
        free((*headers)->headers[i].key);
        free((*headers)->headers[i].value);
    }

    free(*headers);
    *headers = NULL;
}

#pragma endregion

#pragma region Options

static const struct easyhttp_Options EASYHTTP_DEFAULT_OPTIONS = {
    .method = "GET",
    .body = NULL,
    .timeout = 0,
    .follow_redirects = false,
    .max_redirects = -1,
    .headers = NULL
};

static struct easyhttp_Options easyhttp_options_parse(lua_State *L, int idx, const char **error)
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
            const char  *key = luaL_checkstring(L, -2),
                        *value = luaL_checkstring(L, -1);
            size_t key_len = strlen(key), value_len = strlen(value);

            //Only realloc if the new size is greater than the current size, to save on realloc calls
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

        free(buf);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    *error = 0;
    return options;
}

static inline void easyhttp_options_set(struct easyhttp_Options options, CURL *curl)
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

static void easyhttp_options_free(struct easyhttp_Options *options)
{
    curl_slist_free_all(options->headers);
    *options = (struct easyhttp_Options){0};
}

#pragma endregion

#pragma region Buffer
static inline struct easyhttp_Buffer *easyhttp_buffer_create()
{
    struct easyhttp_Buffer *buf = calloc(1, sizeof(struct easyhttp_Buffer) + 1);
    if (!buf) return NULL;
    buf->cap = 1;
    return buf;
}

static struct easyhttp_Buffer *easyhttp_buffer_append(struct easyhttp_Buffer *buffer, size_t size, const char data[static size])
{
    if (buffer->length + size > buffer->cap) {
        size_t new_cap = buffer->cap * 2;
        while (new_cap < buffer->length + size) {
            new_cap *= 2;
        }

        void *tmp = realloc(buffer, sizeof(struct easyhttp_Buffer) + new_cap);
        if (!tmp) {
            return NULL;
        }
        buffer = tmp;
        buffer->cap = new_cap;
    }

    memcpy(buffer->data + buffer->length, data, size);
    buffer->length += size;
    buffer->data[buffer->length] = '\0';
    return buffer;
}

static int easyhttp_buffer_write(void *data, size_t size, size_t nmemb, void *userp)
{
    struct easyhttp_Buffer **buffer = userp;
    size_t new_size = (*buffer)->length + size * nmemb;

    *buffer = easyhttp_buffer_append(*buffer, size * nmemb, data);
    if (!*buffer) {
        return 0;
    }

    return size * nmemb;
}
#pragma endregion

#endif //EASYHTTP_COMMON_H
