---
title: Accessing the myMPD API
---

Calls the myMPD API, look at [API](../../060-references/api/index.md) for detailed API description.

```lua
-- Call myMPD API for current partition
local rc, result = mympd.api("method", params)

-- Call myMPD API for another partition
local rc, result = mympd.api_partition("default", "method", params)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| partition | string | MPD partition |
| method | string | myMPD API method |
| params | lua table | The jsonrpc parameters as Lua table |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = OK, 1 = ERROR |
| result | lua table | json result or error |

Following API methods are not accessible:

- MYMPD_API_SESSION_LOGIN
- MYMPD_API_SESSION_LOGOUT
- MYMPD_API_SESSION_VALIDATE
