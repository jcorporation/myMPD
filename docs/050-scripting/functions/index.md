---
title: Functions
---

List of myMPD specific Lua functions.

| FUNCTION | DESCRIPTION |
| -------- | ----------- |
| [json.decode](json.md) | Parses a Json string to a Lua table. |
| [json.encode](json.md) | Encodes a Lua table as Json string. |
| [mympd.api](mympd_api.md) | Access to the myMPD API. |
| [mympd.cache_cover_write](diskcache.md) | Writes a cover cache file. |
| [mympd.cache_lyrics_write](diskcache.md) | Writes a lyrics cache file. |
| [mympd.cache_thumbs_write](diskcache.md) | Writes a thumbs cache file. |
| [mympd.dialog](mympd_dialog.md) | Returns an Jsonrpc response for a script dialog. |
| [mympd.gpio_blink](gpio.md) | Connects to myGPIOd and blinks a GPIO with given timeout and interval. |
| [mympd.gpio_get](gpio.md) | Connects to myGPIOd and returns the active state of a GPIO. |
| [mympd.gpio_set](gpio.md) | Connects to myGPIOd and sets the active value of a GPIO. |
| [mympd.gpio_toggle](gpio.md) | Connects to myGPIOd and toggles the active value of a GPIO. |
| [mympd.hash_md5](util.md) | MD5 hash of string. |
| [mympd.hash_sha1](util.md) | SHA1 hash of string. |
| [mympd.hash_sha256](util.md) | SHA256 hash of string. |
| [mympd.htmlencode](util.md) | Simple HTML encoding. |
| [mympd.http_client](http_client.md) | Simple HTTP client. |
| [mympd.http_download](http_client.md) | Download a file over http. |
| [mympd.http_jsonrpc_error](http_replies.md) | Sends a JSONRPC 2.0 error. |
| [mympd.http_jsonrpc_response](http_replies.md) | Sends a JSONRPC 2.0 response. |
| [mympd.http_jsonrpc_warn](http_replies.md) | Sends a JSONRPC 2.0 warning. |
| [mympd.http_redirect](http_replies.md) | Returns a valid HTTP redirect message. |
| [mympd.http_reply](http_replies.md) | Returns a valid HTTP response message. |
| [mympd.http_serve_file](http_replies.md) | Serves a file from the filesystem. Only files from the diskcache are allowed. |
| [mympd.init](mympd_init.md) | Initializes the Lua table mympd_state. |
| [mympd.log](util.md) | Logging to myMPD log. |
| [mympd.notify_client](util.md) | Sends a notification to the client. |
| [mympd.notify_partition](util.md) | Sends a notification to all clients in a partition. |
| [mympd.os_capture](system_command.md) | Executes a system command and capture its output. |
| [mympd.tmp_file](diskcache.md) | Generates a random tmp filename for the misc cache. |
| [mympd.update_mtime](diskcache.md) | Updates the timestamp of a file. |
| [mympd.urldecode](util.md) | Decodes a URL encoded string. |
| [mympd.urlencode](util.md) | URL encodes a string. |

- [LuaDoc](../../luadoc/files/release/contrib/lualibs/mympd.html)
