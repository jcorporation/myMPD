---
title: HTTP client
---

## Simple HTTP client

```lua
local rc, code, headers, body = mympd.http_client(method, uri, extra_headers, payload, cache)
if rc == 0 then
  -- Success, iterate through headers
  for name, value in pairs(header) do
    -- Do something
  end
else
  -- Error case
end
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| method | string | HTTP method, e.g. `GET` or `POST` |
| uri | string | Full uri to call, e. g. `https://api.listenbrainz.org/1/submit-listens` |
| extra_headers | string | Additional headers, must be terminated by `\r\n` |
| payload | string | Body of a post request |
| cache | boolean | Optional, cache the response? |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| code | integer | http response code, e.g. 200 |
| headers | table | http headers |
| body | string | http body |

## Download a file over HTTP

If you set the `out` argument to an empty string, the file will be kept in the http client cache. You can serve the file directly from the cache with the `mympd.http_serve_file_from_cache` function.

```lua
local rc, code, headers, filename = mympd.http_download(uri, extra_headers, out, cache)
if rc == 0 then
  -- Do something with the downloaded file
else
  -- Error case
end
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| uri | string | Uri to download. |
| extra_headers | string | Additional headers, must be terminated by `\r\n` |
| out | string | Filename for output. |
| cache | boolean | Optional, cache the response? |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| code | integer | http response code, e.g. 200 |
| headers | table | http headers |
| filename [1] | string | Filepath of downloaded file |

1. The `filename` is set to the `out` argument if it was not empty, else the `filename` is populated with the http client cache filepath.

## Get a http header from the response

Gets the value of a http header. Name of the header is matched case insensitive as per rfc.

```lua
local value = mympd.http_header_get(headers, name)
if value ~= nil then
  -- Header found
else
  -- Header not found
end
```
