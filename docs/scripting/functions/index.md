---
layout: page
permalink: /scripting/functions/
title: Functions
---

List of myMPD specific Lua functions.

| FUNCTION | DESCRIPTION |
| -------- | ----------- |
| [json.decode]({{site.baseurl}}/scripting/functions/json) | Parses a Json string to a Lua table. |
| [json.encode]({{site.baseurl}}/scripting/functions/json) | Encodes a Lua table as Json string. |
| [mympd.api]({{site.baseurl}}/scripting/functions/mympd_api) | Access to the myMPD API. |
| [mympd.cache_cover_write]({{site.baseurl}}/scripting/functions/diskcache) | Writes a cover cache file. |
| [mympd.cache_lyrics_write]({{site.baseurl}}/scripting/functions/diskcache) | Writes a lyrics cache file. |
| [mympd.cache_thumbs_write]({{site.baseurl}}/scripting/functions/diskcache) | Writes a thumbs cache file. |
| [mympd.dialog]({{site.baseurl}}/scripting/functions/mympd_dialog) | Returns an Jsonrpc response for a script dialog. |
| [mympd.gpio_blink]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and blinks a GPIO with given timeout and interval. |
| [mympd.gpio_get]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and returns the active state of a GPIO. |
| [mympd.gpio_set]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and sets the active value of a GPIO. |
| [mympd.gpio_toggle]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and toggles the active value of a GPIO. |
| [mympd.hash_sha1]({{site.baseurl}}/scripting/functions/util) | SHA1 hash of string. |
| [mympd.hash_sha256]({{site.baseurl}}/scripting/functions/util) | SHA256 hash of string. |
| [mympd.http_client]({{site.baseurl}}/scripting/functions/http_client) | Simple HTTP client. |
| [mympd.http_download]({{site.baseurl}}/scripting/functions/http_client) | Download a file over http. |
| [mympd.http_jsonrpc_error]({{site.baseurl}}/scripting/functions/http_replies) | Sends a JSONRPC 2.0 error. |
| [mympd.http_jsonrpc_response]({{site.baseurl}}/scripting/functions/http_replies) | Sends a JSONRPC 2.0 response. |
| [mympd.http_jsonrpc_warn]({{site.baseurl}}/scripting/functions/http_replies) | Sends a JSONRPC 2.0 warning. |
| [mympd.http_redirect]({{site.baseurl}}/scripting/functions/http_replies) | Returns a valid HTTP redirect message. |
| [mympd.http_reply]({{site.baseurl}}/scripting/functions/http_replies) | Returns a valid HTTP response message. |
| [mympd.http_serve_file]({{site.baseurl}}/scripting/functions/http_replies) | Serves a file from the filesystem. Only files from the diskcache are allowed. |
| [mympd.init]({{site.baseurl}}/scripting/functions/mympd_init) | Initializes the Lua table mympd_state. |
| [mympd.log]({{site.baseurl}}/scripting/functions/util) | Logging to myMPD log. |
| [mympd.notify_client]({{site.baseurl}}/scripting/functions/util) | Sends a notification to the client. |
| [mympd.notify_partition]({{site.baseurl}}/scripting/functions/util) | Sends a notification to all clients in a partition. |
| [mympd.os_capture]({{site.baseurl}}/scripting/functions/system_command) | Executes a system command and capture its output. |
| [mympd.tmp_file]({{site.baseurl}}/scripting/functions/diskcache) | Generates a random tmp filename for the misc cache. |
| [mympd.update_mtime]({{site.baseurl}}/scripting/functions/diskcache) | Updates the timestamp of a file. |
| [mympd.urldecode]({{site.baseurl}}/scripting/functions/util) | Decodes a URL encoded string. |
| [mympd.urlencode]({{site.baseurl}}/scripting/functions/util) | URL encodes a string. |
{: .table .table-sm }

- [LuaDoc]({{site.baseurl}}/luadoc/files/debug/contrib/lualibs/mympd.html)
