local easyhttp = require("easyhttp")

local response, code = easyhttp.request("https://httpbin.org/get")
print(response, code)
