# easy-http
Easily create HTTP requests with lua

## Installation
```bash
luarocks install easy-http
```

## Sync Usage

### Simple GET
```lua
local easyhttp = require("easyhttp")

local response, code, headers = easyhttp.request("https://httpbin.org/get")
print(response)
print(code)
print(headers)
```

Output:

```json
{
  "args": {},
  "headers": {
    "Accept": "*/*",
    "Host": "httpbin.org",
    "X-Amzn-Trace-Id": "***"
  },
  "origin": "***",
  "url": "https://httpbin.org/get"
}
200
table: 0x7f81d390a850
```

### POST with custom headers
```lua
local easyhttp = require("easyhttp")

local response, code, headers = easyhttp.request("https://httpbin.org/post", {
    method = "POST",
    headers = {
        ["Content-Type"] = "application/json"
    },
    body = '{"key": "value"}'
})

print(response)
print(code)
print(headers)
```

Response:

```json
{
  "args": {},
  "data": "{\"key\": \"value\"}",
  "files": {},
  "form": {},
  "headers": {
    "Accept": "*/*",
    "Content-Length": "16",
    "Content-Type": "application/json",
    "Host": "httpbin.org",
    "X-Amzn-Trace-Id": "***"
  },
  "json": {
    "key": "value"
  },
  "origin": "***",
  "url": "https://httpbin.org/post"
}

200
table: 0x7f81d2f44910
```

### Timeout
```lua
local easyhttp = require("easyhttp")

local response, code, headers = easyhttp.request("https://httpbin.org/delay/5", {
    timeout = 2,

    --redirects are supported
    follow_redirects = true, --by default true
    max_redirects = 5, --by default, -1 (infinite)
})

print(response)
print(code)
print(headers)
```

Output:

```lua
nil
failed to perform request: Timeout was reached
nil
```

### Output to file
```lua
local easyhttp = require("easyhttp")

local f = assert(io.open("output.json", "w+b"))
local response, code, headers = easyhttp.request("https://httpbin.org/get", {
    output_file = f
})

print(response)
print(code)
print(headers)
```

Output:
```lua.
true
200
table: 0x7fe98630b4c0
```

## Async Usage

### Simple GET
```lua
local easyhttp = require("easyhttp")

local request = assert(easyhttp.async_request("https://httpbin.org/get"))
print(request)
io.write '\27[?25l'
while not request:is_done() do
    io.write '.':flush()
    local t0 = os.clock()
    while os.clock() - t0 <= 0.025 do end
end
io.write '\27[?25h'

local response, code, headers = request:response() --Response will wait until the request is done

print(response)
print(code)
print(headers)
```

Output:
```json
easyhttp.AsyncRequest: 0x7fbd03722a88
{
  "args": {},
  "headers": {
    "Accept": "*/*",
    "Host": "httpbin.org",
    "X-Amzn-Trace-Id": "***"
  },
  "origin": "***",
  "url": "https://httpbin.org/get"
}

200
table: 0x7f81ef10a7e0
```

### Cancel request
```lua
local easyhttp = require("easyhttp")
local request = assert(easyhttp.async_request("https://httpborg/delay/5"))

assert(request:cancel()) --Will return false, <message> if the request is already done

local response, code = request:response()
print(response)
print(code)
```

Output:
```lua
nil
request was cancelled
```
