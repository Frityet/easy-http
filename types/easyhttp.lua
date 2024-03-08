---@meta

---@class easyhttp
---@field public _VERSION "0.1.0"
local easyhttp = {}

---@alias easyhttp.HTTPMethod
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
---@field output_file file*?
---@field on_progress (fun(dltotal: number, dlnow: number, ultotal: number, ulnow: number): number?)?
---@field on_data (fun(data: string, size: integer, nmemb: integer): string | false | nil)?


---Sends a synchronous HTTP request, blocking the current thread until the request is complete.
---@param url string
---@param options easyhttp.RequestOptions?
---@return (string | true)? body, integer | string? code, { [string] : string }? headers
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

---Gets the progress of the request
---@return number dltotal, number dlnow, number ultotal, number ulnow
function AsyncRequest:progress() end

---Gets the data of the request (if any). This call does not block, so if it is called before `:is_done()` the data will likley be incomplete.
---@return string? data, integer? size
function AsyncRequest:data() end

---Sends an asynchronous HTTP request, returning an AsyncRequest object.
---@param url string
---@param options easyhttp.RequestOptions?
---@return easyhttp.AsyncRequest? request, string? error
function easyhttp.async_request(url, options) end

return easyhttp
