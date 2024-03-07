local easyhttp = require("easyhttp")
local request = assert(easyhttp.async_request("https://njfenjerfnooerfoiernobfoberfboeoibfreboreffrbijoburevbouev.com"))
local response, code = request:response()
print(response, code)
