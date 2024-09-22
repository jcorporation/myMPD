---

title: Accessing the myMPD API
---

Calls the myMPD API, look at [API](../../060-references/api/index.md) for detailed API description.

```lua
local rc, result = mympd.api("method", params)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| method | string | myMPD API method |
| params | lua table | the jsonrpc parameters |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = OK, 1 = ERROR |
| result | lua table | json result or error |

Following API methods are not accessible:

- MYMPD_API_SESSION_LOGIN
- MYMPD_API_SESSION_LOGOUT
- MYMPD_API_SESSION_VALIDATE
