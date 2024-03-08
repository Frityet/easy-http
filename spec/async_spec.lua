-- Copyright (c) 2024 Amrit Bhogal
--
-- This software is released under the MIT License.
-- https://opensource.org/licenses/MIT

describe("async request", function ()
    it("should exist", function ()
        local easyhttp = require("easyhttp")
        assert.truthy(easyhttp.async_request)
    end)

    it("should be a function", function ()
        local easyhttp = require("easyhttp")
        assert.is_function(easyhttp.async_request)
    end)

    it("should return a request object", function ()
        local easyhttp = require("easyhttp")
        local request = easyhttp.async_request("https://httpbin.org/get")
        assert.truthy(request)
        local tname = tostring(request)
        --make sure the first part is easyhttp.AsyncRequest
        --only if lua _VERSION is 5.4 will `tosring` return the type name, on <=5.3 it will return `userdata`
        if _VERSION == "Lua 5.4" then
            assert.are_equal("easyhttp.AsyncRequest", tname:sub(1, #"easyhttp.AsyncRequest"))
        else
            assert.are_equal("userdata", tname:sub(1, #"userdata"))
        end
    end)

    describe("is done", function ()
        it("should exist", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.truthy(request.is_done)
        end)

        it("should be a function", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.is_function(request.is_done)
        end)

        it("should return false", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/delay/10")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.falsy(request:is_done())
        end)

        it("should return true", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            local response = request:response()
            assert.truthy(response)
            assert.truthy(request:is_done())
        end)
    end)

    describe("response", function ()
        it("should exist", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.truthy(request.response)
        end)

        it("should be a function", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.is_function(request.response)
        end)

        it("should return a string, integer, table", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            local response, code, headers = request:response()
            assert.is_string(response)
            assert.is_number(code)
            assert.is_table(headers)
        end)

        it("should return 404 for a non-existent page", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/status/404")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            local response, code = request:response()
            assert.are_equal(404, code)
        end)

        it("should return error for an unresolved domain", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://njfenjerfnooerfoiernobfoberfboeoibfreboreffrbijoburevbouev.com")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            local response, code = request:response()
            assert.is_nil(response)
            assert.is_string(code)
        end)
    end)

    describe("cancel", function ()
        it("should exist", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.truthy(request.cancel)
        end)

        it("should be a function", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.is_function(request.cancel)
        end)

        it("should cancel the request", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/delay/5")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.is_truthy(request:cancel())
            local response, code = request:response()
            assert.is_nil(response)
            assert.are_equal("request was cancelled", code)
        end)

        it("should return false if request is complete", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            while not request:is_done() do
                --wait
            end
            assert.is_falsy(request:cancel())
        end)
    end)

    describe("progress", function ()
        it("should exist", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.truthy(request.progress)
        end)

        it("should be a function", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.is_function(request.progress)
        end)

        it("should return 4 numbers", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            local dltotal, dlnow, ultotal, ulnow = request:progress()
            assert.is_number(dltotal)
            assert.is_number(dlnow)
            assert.is_number(ultotal)
            assert.is_number(ulnow)
        end)
    end)

    describe("data", function ()
        it("should exist", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.truthy(request.data)
        end)

        it("should be a function", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            assert.is_function(request.data)
        end)

        it("should return a string?, integer?", function ()
            local easyhttp = require("easyhttp")
            local request = easyhttp.async_request("https://httpbin.org/get")
            assert.truthy(request)
            --[[@cast request easyhttp.AsyncRequest]]
            local data, size = request:data()
            local data_t, size_t = type(data), type(size)
            assert.truthy(data_t == "string" or data_t == "nil")
            assert.truthy(size_t == "number" or size_t == "nil")
        end)
    end)
end)

