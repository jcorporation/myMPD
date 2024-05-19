---
layout: page
permalink: /scripting/functions/mympd_api
title: Utility Functions
---

Some useful utility functions.

## Hashing functions

```lua
sha1_hash = mympd.hash_sha1(string)
sha256_hash = mympd.hash_sha256(string)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to hash |
{: .table .table-sm }

## URL encoding and decoding

```lua
encoded = mympd.urlencode(string)
decoded = mympd.urldecode(string, form_url_decode)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to encode/decode |
| form_url_decode | boolean | Decode as form url |
{: .table .table-sm }

## Logging

Logs messages to the myMPD log.

```lua
mympd.log(loglevel, message)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| message | string | Message to log |
| loglevel | number | Syslog log level |
{: .table .table-sm }

| LOGLEVEL | DESCRIPTION |
| -------- | ----------- |
| 0 | Emergency |
| 1 | Alert |
| 2 | Critical |
| 3 | Error |
| 4 | Warning |
| 5 | Notice |
| 6 | Info |
| 7 | Debug |
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
| severity | number | 0 = info, 1 = warn, 2 = error|
| message | string | Message to send. |