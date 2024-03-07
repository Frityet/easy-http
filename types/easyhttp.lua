---@meta

--[[
```tl
local record easyhttp
    enum HTTPMethod
        "GET"
        "POST"
        "PUT"
        "DELETE"
        "PATCH"
        "HEAD"
        "OPTIONS"
    end

    record RequestOptions
        method: HTTPMethod
        headers: {string:string}
        body: string
        timeout: number
        follow_redirects: boolean
        max_redirects: number
    end

    request: function(url: string, options: RequestOptions): string | nil, integer | string, {string:string} | nil

    record AsyncRequest
        is_done: function(AsyncRequest): boolean
        response: function(AsyncRequest): string | nil, integer | string, {string:string} | nil
        cancel: function(AsyncRequest): boolean, string | nil
    end

    async_request: function(url: string, options: RequestOptions): AsyncRequest | nil, string | nil
end

return easyhttp

```
]]
---@class easyhttp
local easyhttp = {}

---@enum easyhttp.HTTPMethod
---| '"GET"'
---| '"POST"'
---| '"PUT"'
---| '"DELETE"'
---| '"PATCH"'
---| '"HEAD"'
---| '"OPTIONS"'

---@class easyhttp.RequestOptions
---@field method easyhttp.HTTPMethod?
---@field headers { [string] : string }?
---@field body string?
---@field timeout number?
---@field follow_redirects boolean?
---@field max_redirects number?

---Sends a synchronous HTTP request, blocking the current thread until the request is complete.
---@param url string
---@param options easyhttp.RequestOptions
---@return string? body, integer | string? code, { [string] : string }? headers
function easyhttp.request(url, options) end

---@class easyhttp.AsyncRequest
local AsyncRequest = {}

---Returns true if the request is complete, false otherwise.
---@return boolean
function AsyncRequest:is_done() end

---Gets the response, same return values as easyhttp.request.
---@return string? body, integer | string? code, { [string] : string }? headers
function AsyncRequest:response() end

---Cancels the request, returns true if the request was successfully cancelled, false otherwise, and why it was not cancelled.
---@return boolean, string?
function AsyncRequest:cancel() end

---Sends an asynchronous HTTP request, returning an AsyncRequest object.
---@param url string
---@param options easyhttp.RequestOptions
---@return easyhttp.AsyncRequest? request, string? error
function easyhttp.async_request(url, options) end

return easyhttp
