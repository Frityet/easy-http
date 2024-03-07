/**
 * Copyright (c) 2024 Amrit Bhogal
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "async.h"

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

enum {
    EASYHTTP_EXIT_CANCELLED = 0xA,
    EASYHTTP_EXIT_ERROR = 0xB,
};

static int handle_error(struct easyhttp_AsyncRequest *request, const char *error)
{
    mtx_lock(&request->mutex);
    {
        if (request->cancelled) {
            mtx_unlock(&request->mutex);
            return 1;
        }

        request->error = error;
        request->cancelled = true;
    }
    mtx_unlock(&request->mutex);
    return EASYHTTP_EXIT_ERROR;
}

static const char *thread_error_message(int i)
{
    const char *error_reason = NULL;
    switch (i) {
        case thrd_nomem:
            error_reason = "out of memory";
            break;
        case thrd_timedout:
            error_reason = "timed out";
            break;
        case thrd_busy:
            error_reason = "resource busy";
            break;
        default:
            error_reason = "unknown error";
            break;
    }
    return error_reason;
}

static int buffer_write(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct easyhttp_AsyncRequest *request = userp; //to check `cancel` flag
    if (request->cancelled) {
        thrd_exit(EASYHTTP_EXIT_CANCELLED);

        __extension__ __builtin_unreachable();
        return 0;
    }

    mtx_lock(&request->mutex);
    int ret = easyhttp_buffer_write(ptr, size, nmemb, &request->request.response);
    mtx_unlock(&request->mutex);
    return ret;
}

static int easyhttp_thread_func(void *ptr)
{
    struct easyhttp_AsyncRequest *req = ptr;
    CURL *curl = curl_easy_init();
    if (!curl) {
        return handle_error(req, "failed to create curl handle");
    }

    easyhttp_set_options(req->request.options, curl);

    curl_easy_setopt(curl, CURLOPT_URL, req->request.url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write);

    req->request.response = easyhttp_buffer_create();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return handle_error(req, curl_easy_strerror(res));
    }

    mtx_lock(&req->mutex);
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &req->request.response_code);

        //get headers
        struct curl_header *header = NULL;
        while ((header = curl_easy_nextheader(curl, CURLH_HEADER, -1, header))) {
            const char *key = header->name, *value = header->value;
            req->request.header_count++;
            req->request.headers = realloc(req->request.headers, req->request.header_count * sizeof(*req->request.headers));
            if (!req->request.headers) {
                return handle_error(req, "failed to allocate memory for headers");
            }
            req->request.headers[req->request.header_count - 1].key = strdup(key);
            if (!req->request.headers[req->request.header_count - 1].key) {
                return handle_error(req, "failed to allocate memory for header key");
            }
            req->request.headers[req->request.header_count - 1].value = strdup(value);
            if (!req->request.headers[req->request.header_count - 1].value) {
                return handle_error(req, "failed to allocate memory for header value");
            }
        }
        req->done = true;
    }
    mtx_unlock(&req->mutex);
    return 0;
}

/*
function easyhttp.async_request(url: string, {
    method: "GET" | "POST" | "PUT" | "DELETE" | string = "GET",
    headers: { [string]: string }?,
    body: string?,
    timeout: number = 30,
    follow_redirects: boolean = true,
    max_redirects: number?,
}?, callback: function(body: string?, status_code: integer, headers: { [string]: string })): easyhttp.AsyncRequest?, string? error
*/
int easyhttp_async_request(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = lua_newuserdata(L, sizeof(struct easyhttp_AsyncRequest));
    luaL_setmetatable(L, EASYHTTP_ASYNC_REQUEST_TNAME);

    request->request.url = luaL_checkstring(L, 1);

    const char *err = NULL;
    request->request.options = easyhttp_parse_options(L, 2, &err);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }

    int i = mtx_init(&request->mutex, mtx_plain);
    if (i != thrd_success) {
        lua_pushnil(L);
        lua_pushliteral(L, "failed to create mutex");
        return 2;
    }

    i = thrd_create(&request->thread, easyhttp_thread_func, request);
    if (i != thrd_success) {
        lua_pushnil(L);
        lua_pushfstring(L, "failed to create thread: %s", thread_error_message(i));
        return 2;
    }

    return 1;
}

int easyhttp_async_request_is_done(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    mtx_lock(&request->mutex);
    bool done = request->done;
    mtx_unlock(&request->mutex);
    lua_pushboolean(L, done);
    return 1;
}

// function easyhttp.async_request_response(request: easyhttp.AsyncRequest): (string body, integer status_code, { [string]: string } headers) | (nil, string error)
int easyhttp_async_request_response(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    mtx_lock(&request->mutex);
    if (request->error) {
        lua_pushnil(L);
        lua_pushstring(L, request->error);
        mtx_unlock(&request->mutex);
        return 2;
    }

    if (!request->done) {
        int res = 0;
        mtx_unlock(&request->mutex); //so that we can join the thread without deadlock
        int c = thrd_join(request->thread, &res);
        if (c != thrd_success) {
            lua_pushnil(L);
            lua_pushfstring(L, "failed to join thread: %s", thread_error_message(c));
            mtx_unlock(&request->mutex);
            return 2;
        }

        if (res != 0) {
            lua_pushnil(L);
            switch (res) {
                case EASYHTTP_EXIT_CANCELLED:
                    lua_pushliteral(L, "request was cancelled");
                    break;
                case EASYHTTP_EXIT_ERROR:
                    lua_pushstring(L, request->error);
                    break;
                default:
                    lua_pushliteral(L, "unknown error");
                    break;
            }
            return 2;
        }
    }

    lua_pushlstring(L, request->request.response->data, request->request.response->size);
    lua_pushinteger(L, request->request.response_code);
    lua_newtable(L);
    for (size_t i = 0; i < request->request.header_count; i++) {
        lua_pushstring(L, request->request.headers[i].value);
        lua_setfield(L, -2, request->request.headers[i].key);
    }
    mtx_unlock(&request->mutex);
    return 3;
}

int easyhttp_async_request_cancel(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    mtx_lock(&request->mutex);
    if (request->done) {
        mtx_unlock(&request->mutex);
        lua_pushboolean(L, false);
        lua_pushliteral(L, "request is already done");
        return 2;
    }
    request->cancelled = true;
    mtx_unlock(&request->mutex);

    lua_pushboolean(L, true);
    return 1;
}

int easyhttp_async_request__gc(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    request->cancelled = true;
    thrd_join(request->thread, NULL);

    mtx_destroy(&request->mutex);

    free(request->request.options.headers);
    free(request->request.response);

    if (request->request.headers) {
        for (size_t i = 0; i < request->request.header_count; i++) {
            free(request->request.headers[i].key);
            free(request->request.headers[i].value);
        }
        free(request->request.headers);
    }

    return 0;
}
