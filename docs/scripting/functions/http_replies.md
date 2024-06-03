---
layout: page
permalink: /scripting/functions/http_replies
title: HTTP Replies
---

## HTTP replies

This functions are creating raw http response for usage in scripts called by http requests.

Such scripts can be called by the URI `/script/<partition>/<script>`.

```lua
-- Return a complete http reply
local status = 200
local headers ="Content-type: text/plain\r\n"
local body = "testbody"
return mympd.http_reply(status, headers, body)

-- Return a 302 FOUND response (temporary redirect) to /test
local location = "/test"
return mympd.http_redirect(location)

-- Serve a file from the cache
local file = mympd_env.cachedir_misc .. "/test.png"
return mympd.http_serve_file(file)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| status | integer | HTTP status code, e.g. 200 |
| headers | string | HTTP headers to append, terminate each header with `\r\n`. `Status`, `Connection` and `Content-Length` headers are added automatically. |
| body | string | Response body |
{: .table .table-sm }

## JSONRPC

## Send a JSONRPC 2.0 result

```lua
local result = {
  data = [{
    synced = false,
    lang = "",
    desc = "",
    text = "Script generated lyrics"
  }],
  totalEntities = 1,
  returnedEntities = 1
}
return mympd.http_jsonrpc_response(result)
```

## Send a JSONRPC 2.0 error or warning

```lua
return mympd.http_jsonrpc_error(method, msg)
return mympd.http_jsonrpc_warn(method, msg)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| method | integer | myMPD API method |
| msg | string | Error message |
{: .table .table-sm }
