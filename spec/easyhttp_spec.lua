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
            local response, code, headers = easyhttp.request("https://httpbin.org/get")
            assert.truthy(response)
            assert.are_equal(200, code)
        end)

        it("should return a string, integer, and table", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://httpbin.org/get")
            assert.is_string(response)
            assert.is_number(code)
            assert.is_table(headers)
        end)

        it("should return a table of headers", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://httpbin.org/get")
            assert.is_truthy(response)
            assert.are_equal(200, code)
            --[[@cast headers {[string]:string}]]
            assert.are_equal("application/json", headers["content-type"])
        end)

        it("can write output to a file", function ()
            local easyhttp = require("easyhttp")
            local f, err = io.open("test.json", "w+b")
            assert.truthy(f and not err)
            local response, code, headers = easyhttp.request("https://httpbin.org/get", {
                output_file = f
            })
            --[[@cast f file*]]
            local ok, err = f:close()
            assert.is_true(response)
            assert.are_equal(200, code)
            assert.truthy(ok and not err)
            os.remove("test.json")
        end)

        it("should allow for custom headers", function ()
            local easyhttp = require("easyhttp")
            local json = require("dkjson")
            local response, code, headers = easyhttp.request("https://httpbin.org/get", {
                headers = {
                    ["User-Agent"] = "easyhttp"
                }
            })
            assert.truthy(response)
            assert.are_equal(200, code)

            --[[@cast response string]]
            local data, _, err = json.decode(response)
            assert.truthy(data)
            --[[@cast data table]]
            assert.are_equal("easyhttp", data.headers["User-Agent"])
        end)

        it("should return 404 for a non-existent page", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://httpbin.org/status/404")
            assert.truthy(response)
            assert.are_equal(404, code)
        end)

        it("should return error for an unresolved domain", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://njfenjerfnooerfoiernobfoberfboeoibfreboreffrbijoburevbouev.com")
            assert.falsy(response)
            assert.is_string(code)
        end)

        -- it("can redirect", function ()
        --     local easyhttp = require("easyhttp")
        --     local response, code, headers = easyhttp.request("https://httpbin.org/redirect/1")
        --     assert.truthy(response)
        --     assert.truthy(code == 301 or code == 302)
        -- end)

        -- it("shouldn't follow redirects on follow_redirects = false", function ()
        --     local easyhttp = require("easyhttp")
        --     local response, code, headers = easyhttp.request("https://httpbin.org/redirect/1", {
        --         follow_redirects = false
        --     })
        --     assert.truthy(response)
        --     assert.truthy(code == 301 or code == 302)
        -- end)

        -- it("cancel on too many redirects", function ()
        --     local easyhttp = require("easyhttp")
        --     local response, code, headers = easyhttp.request("https://httpbin.org/redirect/10", {
        --         max_redirects = 5
        --     })
        --     assert.truthy(response)
        --     assert.truthy(code == 301 or code == 302)
        -- end)
    end)

    describe("post", function ()
        it("should make a post request", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://httpbin.org/post", {
                method = "POST",
                body = "Hello, World!"
            })
            assert.truthy(response)
            assert.are_equal(200, code)
        end)

        it("should return a string, integer, and table", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://httpbin.org/post", {
                method = "POST",
                body = "Hello, World!"
            })

            assert.is_string(response)
            assert.is_number(code)
            assert.is_table(headers)
        end)

        it("should return a table of headers", function ()
            local easyhttp = require("easyhttp")
            local json = require("dkjson")
            local response, code, headers = easyhttp.request("https://httpbin.org/post", {
                method = "POST",
                body = "data=Hello, World!"
            })
            assert.truthy(response)
            --[[@cast response string]]
            assert.are_equal(200, code)
            --[[@cast headers {[string]:string}]]
            assert.are_equal("application/json", headers["content-type"])
            assert.are_equal(code, 200)
            local data, _, err = json.decode(response)
            assert.truthy(data)
            --[[@cast data table]]
            assert.are_equal("Hello, World!", data.form.data)
        end)
    end)

    it("should timeout", function ()
        local easyhttp = require("easyhttp")
        local response, code, headers = easyhttp.request("https://httpbin.org/delay/10", {
            timeout = 1
        })
        assert.falsy(response)
        --[[@cast code string]]
        assert.are_same("failed to perform request: Timeout was reached", code)
    end)

    describe("callbacks", function()
        it("should allow for the on_data callback", function ()
            local easyhttp = require("easyhttp")
            local cb_called = false
            local response, code, headers = easyhttp.request("https://httpbin.org/get", {
                on_data = function (data, size)
                    cb_called = true
                    assert.is_string(data)
                    assert.is_number(size)
                end
            })
            assert.truthy(response)
            assert.are_equal(200, code)
            assert.truthy(cb_called)
        end)

        it("should cancel the request if on_data returns false", function ()
            local easyhttp = require("easyhttp")
            local ran_once = false
            local response, code, headers = easyhttp.request("https://hil-speed.hetzner.com/100MB.bin", {
                on_data = function ()
                    assert.falsy(ran_once)
                    ran_once = true
                    return false
                end
            })
            assert.falsy(response)
            assert.are_same("failed to perform request: Failed writing received data to disk/application", code)
        end)

        it("should allow for the on_progress callback", function ()
            local easyhttp = require("easyhttp")
            local cb_called = false
            local response, code, headers = easyhttp.request("https://hil-speed.hetzner.com/100MB.bin", {
                on_progress = function (dltotal, dlnow, ultotal, ulnow)
                    cb_called = true
                    assert.is_number(dltotal)
                    assert.is_number(dlnow)
                    assert.is_number(ultotal)
                    assert.is_number(ulnow)
                end
            })
            assert.truthy(response)
            assert.are_equal(200, code)
            assert.truthy(cb_called)
        end)

        it("should cancel the request if result is non-zero", function ()
            local easyhttp = require("easyhttp")
            local response, code, headers = easyhttp.request("https://hil-speed.hetzner.com/100MB.bin", {
                on_progress = function ()
                    return 1
                end
            })
            assert.falsy(response)
            assert.are_same("failed to perform request: Operation was aborted by an application callback", code)
        end)
    end)
end)

