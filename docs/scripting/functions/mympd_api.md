---
layout: page
permalink: /scripting/functions/mympd_api
title: Accessing the myMPD API
---

Calls the myMPD API, look at [API]({{ site.baseurl }}/references/api/) for detailed API description.

```lua
local rc, result = mympd.api("method", params)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| method | string | myMPD API method |
| params | lua table | the jsonrpc parameters |
{: .table .table-sm }

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = OK, 1 = ERROR |
| result | lua table | json result or error |
{: .table .table-sm }

Following API methods are not accessible:

- MYMPD_API_SESSION_LOGIN
- MYMPD_API_SESSION_LOGOUT
- MYMPD_API_SESSION_VALIDATE
- MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT
- MYMPD_API_CLOUD_RADIOBROWSER_NEWEST
- MYMPD_API_CLOUD_RADIOBROWSER_SERVERLIST
- MYMPD_API_CLOUD_RADIOBROWSER_SEARCH
- MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL
- MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET
