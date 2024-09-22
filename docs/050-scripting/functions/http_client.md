---

title: HTTP client
---

## Simple HTTP client

```lua
local rc, code, headers, body = mympd.http_client(method, uri, extra_headers, payload)
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
| method | string | HTTP method, `GET` or `POST` |
| uri | string | Full uri to call, e. g. `https://api.listenbrainz.org/1/submit-listens` |
| extra_headers | string | Additional headers, must be terminated by `\r\n` |
| payload | string | body of a post request |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| code | integer | http response code, e.g. 200 |
| headers | table | http headers |
| body | string | http body |

## Download a file over HTTP

```lua
local rc, code, headers = mympd.http_download(uri, extra_headers, out)
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

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| code | integer | http response code, e.g. 200 |
| headers | table | http headers |

