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
        output_file: FILE
    end

    request: function(url: string, options: RequestOptions | nil): string | boolean | nil, integer | string, {string:string} | nil

    record AsyncRequest
        is_done: function(AsyncRequest): boolean
        response: function(AsyncRequest): string | nil, integer | string, {string:string} | nil
        cancel: function(AsyncRequest): boolean, string | nil
    end

    async_request: function(url: string, options: RequestOptions | nil): AsyncRequest | nil, string | nil
end

return easyhttp
