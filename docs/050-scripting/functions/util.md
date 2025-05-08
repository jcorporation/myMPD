---
title: Utility Functions
---

Some useful utility functions.

## Check arguments

Checks arguments from the mympd_arguments global variable.

```lua
local tocheck = {
    uri = "notempty",
    entries = "number",
    view = "required"
}
local rc, msg = mympd.check_arguments(tocheck)
if rc == false then
    return msg
end
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| tocheck | table | Lua table of arguments to check. |

The `tocheck` parameter expects a lua table consisting of arguments to check as key and one of `notempty`, `number` or `required` as value.

- `notempty`: Checks for a not empty string.
- `number`: Checks if the argument can be converted to a Lua number.
- `required`: Checks if the argument is there.

## Hashing functions

```lua
local md5_hash = mympd.hash_md5(string)
local sha1_hash = mympd.hash_sha1(string)
local sha256_hash = mympd.hash_sha256(string)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to hash |

## HTML encoding

```lua
local encoded = mympd.htmlencode(string)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to encode |

## URL encoding and decoding

```lua
local encoded = mympd.urlencode(string)
local decoded = mympd.urldecode(string, form_url_decode)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to encode/decode |
| form_url_decode | boolean | Decode as form url |

## Logging

Logs messages to the myMPD log. You can use the number or the loglevel name.

```lua
mympd.log(loglevel, message)

-- This is the same
mympd.log(5, message)
mympd.log("LOG_NOTICE", message)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| message | string | Message to log |
| loglevel | number or string | Syslog log level |

| LOGLEVEL | NUMBER | DESCRIPTION |
| -------- | ------ | ----------- |
| LOG_EMERG | 0 | Emergency |
| LOG_ALERT | 1 | Alert |
| LOG_CRIT | 2 | Critical |
| LOG_ERR | 3 | Error |
| LOG_WARNING | 4 | Warning |
| LOG_NOTICE | 5 | Notice |
| LOG_INFO | 6 | Informational |
| LOG_DEBUG | 7 | Debug |

## Return message from script

```lua
local msg = "test"

-- Debug
return mympd.jsonrpc_notification(7, msg)

-- Info
return msg

-- Notice
return mympd.jsonrpc_notification(5, msg)

-- Warning
return mympd.jsonrpc_warn(msg)

-- Error
return mympd.jsonrpc_error(msg)

-- Critical
return mympd.jsonrpc_notification(2, msg)

-- Alert
return mympd.jsonrpc_notification(1, msg)

-- Emergency
return mympd.jsonrpc_notification(0, msg)
```

## Notifications

```lua
-- Send a notification to the client that has started the script
mympd.notify_client(severity, message)

-- Send a notification to all clients in the partition from which the client started the script
mympd.notify_partition(severity, message)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| severity | number or string | Severity |
| message | string | Message to send. |

| SEVERITY | NUMBER | DESCRIPTION |
| -------- | ------ | ----------- |
| SEVERITY_EMERG | 0 | Emergency |
| SEVERITY_ALERT | 1 | Alert |
| SEVERITY_CRIT | 2 | Critical |
| SEVERITY_ERR | 3 | Error |
| SEVERITY_WARNING | 4 | Warning |
| SEVERITY_NOTICE | 5 | Notice |
| SEVERITY_INFO | 6 | Informational |
| SEVERITY_DEBUG | 7 | Debug |

## Read an ascii file

```lua
local content = mympd.read_file(path)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| path | string | Filepath to open and read. |

## Remove a file

Deletes a file or empty directory. It is a wrapper for `os.remove` that logs the error on failure.

```lua
local rc, errorstr = mympd.remove_file(path)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| path | string | Filepath to delete. |

## Sleep

```lua
mympd.sleep(ms)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| ms | positive number | Milliseconds |
