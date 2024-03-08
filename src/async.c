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
        return 0;
    }

    mtx_lock(&request->mutex);
    int ret = easyhttp_buffer_write(ptr, size, nmemb, &request->request.response);
    mtx_unlock(&request->mutex);
    return ret;
}

static size_t header_write(char *buf, size_t size, size_t nmemb, void *userp)
{
    struct easyhttp_AsyncRequest *request = userp; //to check `cancel` flag
    if (request->cancelled) {
        thrd_exit(EASYHTTP_EXIT_CANCELLED);
        return 0;
    }

    mtx_lock(&request->mutex);
    char *header = string_duplicate_n(buf, size * nmemb);
    if (!header) {
        return handle_error(request, "failed to allocate memory for header"), 0;
    }


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

    request->request.headers = easyhttp_headers_append(request->request.headers, header, value);
    if (!request->request.headers || !request->request.headers->headers[request->request.headers->length - 1].key || !request->request.headers->headers[request->request.headers->length - 1].value) {
        free(header);
        return handle_error(request, "failed to allocate memory for header kv pairs"), 0;
    }

    mtx_unlock(&request->mutex);
    return size * nmemb;
}

static int progress_callback(struct easyhttp_AsyncRequest *data, double dltotal, double dlnow, double ultotal, double ulnow)
{
    mtx_lock(&data->mutex);
    if (data->cancelled) {
        mtx_unlock(&data->mutex);
        thrd_exit(EASYHTTP_EXIT_CANCELLED);
        return 0;
    }

    data->request.progress.dlnow = dlnow;
    data->request.progress.dltotal = dltotal;
    data->request.progress.ulnow = ulnow;
    data->request.progress.ultotal = ultotal;

    mtx_unlock(&data->mutex);
    return 0;
}

static int easyhttp_thread_func(void *ptr)
{
    struct easyhttp_AsyncRequest *req = ptr;
    CURL *curl = curl_easy_init();
    if (!curl) {
        return handle_error(req, "failed to create curl handle");
    }

    easyhttp_options_set(req->request.options, curl);

    curl_easy_setopt(curl, CURLOPT_URL, req->request.url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write);

    req->request.response = easyhttp_buffer_create();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);

    req->request.headers = easyhttp_headers_create();
    if (!req->request.headers) {
        return handle_error(req, "failed to allocate memory for headers");
    }


    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return handle_error(req, curl_easy_strerror(res));
    }

    mtx_lock(&req->mutex);
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &req->request.response_code);
        req->done = true;
    }
    mtx_unlock(&req->mutex);
    return 0;
}

int easyhttp_async_request(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = lua_newuserdata(L, sizeof(struct easyhttp_AsyncRequest));
    *request = (struct easyhttp_AsyncRequest) {0};
    luaL_setmetatable(L, EASYHTTP_ASYNC_REQUEST_TNAME);

    request->request.url = luaL_checkstring(L, 1);

    const char *err = NULL;
    request->request.options = easyhttp_options_parse(L, 2, &err);
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

    lua_pushlstring(L, request->request.response->data, request->request.response->length);
    lua_pushinteger(L, request->request.response_code);
    lua_newtable(L);
    for (size_t i = 0; i < request->request.headers->length; i++) {
        lua_pushstring(L, request->request.headers->headers[i].value);
        lua_setfield(L, -2, request->request.headers->headers[i].key);
    }
    mtx_unlock(&request->mutex);
    return 3;
}

int easyhttp_async_request_progress(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    mtx_lock(&request->mutex);
    lua_pushnumber(L, request->request.progress.dlnow);
    lua_pushnumber(L, request->request.progress.dltotal);
    lua_pushnumber(L, request->request.progress.ulnow);
    lua_pushnumber(L, request->request.progress.ultotal);
    mtx_unlock(&request->mutex);
    return 4;
}

int easyhttp_async_request_data(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);
    mtx_lock(&request->mutex);
    if (request->request.response) {
        lua_pushlstring(L, request->request.response->data, request->request.response->length);
        lua_pushinteger(L, request->request.response->length);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    mtx_unlock(&request->mutex);
    return 2;
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

//if extensions
#if defined(__GNUC__)
__attribute__((no_sanitize("address", "thread")))
#endif
int easyhttp_async_request__gc(lua_State *L)
{
    struct easyhttp_AsyncRequest *request = luaL_checkudata(L, 1, EASYHTTP_ASYNC_REQUEST_TNAME);

    request->cancelled = true;
    int res = 0;
    //if sanitizers are enabled, just don't join, it doesn't work for some reason
#if !defined(__SANITIZE_ADDRESS__)
    int err = thrd_join(request->thread, &res);
    if (err != thrd_success) {
        return luaL_error(L, "failed to join thread: %s", thread_error_message(err));
    }
#endif
    mtx_destroy(&request->mutex);

    easyhttp_options_free(&request->request.options);
    free(request->request.response);
    easyhttp_headers_free(&request->request.headers);

    return 0;
}
