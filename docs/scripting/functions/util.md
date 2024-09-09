---
layout: page
permalink: /scripting/functions/util
title: Utility Functions
---

Some useful utility functions.

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
{: .table .table-sm }

## HTML encoding

```lua
local encoded = mympd.htmlencode(string)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to encode |
{: .table .table-sm }

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
{: .table .table-sm }

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
{: .table .table-sm }

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
{: .table .table-sm }

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
{: .table .table-sm }

| SEVERITY | NUMBER | DESCRIPTION |
| -------- | ------ | ----------- |
| SEVERITY_INFO | 0 | Informational |
| SEVERITY_WARNING | 1 | Warning |
| SEVERITY_ERR | 2 | Error |
{: .table .table-sm }