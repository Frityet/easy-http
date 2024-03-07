-- Copyright (c) 2024 Amrit Bhogal
--
-- This software is released under the MIT License.
-- https://opensource.org/licenses/MIT

describe("init", function ()
    it("should exist", function ()
        local easyhttp = require("easyhttp")
        assert.truthy(easyhttp)
    end)
end)

describe("_VERSION", function ()
    it("should exist", function ()
        local easyhttp = require("easyhttp")
        assert.truthy(easyhttp._VERSION)
    end)

    it("should return a string", function ()
        local easyhttp = require("easyhttp")
        assert.is_string(easyhttp._VERSION)
    end)
end)

describe("request", function ()
    it("should exist", function ()
        local easyhttp = require("easyhttp")
        assert.truthy(easyhttp.request)
    end)

    it("should be a function", function ()
        local easyhttp = require("easyhttp")
        assert.is_function(easyhttp.request)
    end)

    describe("get", function ()
        it("should make a get request", function ()
            local easyhttp = require("easyhttp")
            local response, code = easyhttp.request("https://httpbin.org/get")
            assert.truthy(response and code == 200)
        end)
    end)
end)
