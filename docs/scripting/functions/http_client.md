---
layout: page
permalink: /scripting/functions/http_client
title: HTTP client
---

A simple http client.

```lua
rc, code, header, body = mympd.http_client(method, uri, headers, payload)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| method | string | HTTP method, `GET` or `POST` |
| uri | string | full uri to call, e. g. `https://api.listenbrainz.org/1/submit-listens` |
| headers | string | must be terminated by `\r\n` |
| payload | string | body of a post request |
{: .table .table-sm }

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = success, 1 = error |
| code | integer | http response code, e.g. 200 |
| header | string | http headers |
| body | string | http body |
{: .table .table-sm }
