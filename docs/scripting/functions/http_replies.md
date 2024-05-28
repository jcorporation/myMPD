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
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| status | integer | HTTP status code, e.g. 200 |
| headers | string | HTTP headers to append, terminate each header with `\r\n`. `Status`, `Connection` and `Content-Length` headers are added automatically. |
| body | string | Response body |
{: .table .table-sm }

## JSONRPC reply

Sends a JSONRPC 2.0 reply.

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
