---

title: Functions
---

List of myMPD specific Lua functions.

| FUNCTION | DESCRIPTION |
| -------- | ----------- |
| [json.decode](scripting/functions/json) | Parses a Json string to a Lua table. |
| [json.encode](scripting/functions/json) | Encodes a Lua table as Json string. |
| [mympd.api](scripting/functions/mympd_api) | Access to the myMPD API. |
| [mympd.cache_cover_write](scripting/functions/diskcache) | Writes a cover cache file. |
| [mympd.cache_lyrics_write](scripting/functions/diskcache) | Writes a lyrics cache file. |
| [mympd.cache_thumbs_write](scripting/functions/diskcache) | Writes a thumbs cache file. |
| [mympd.dialog](scripting/functions/mympd_dialog) | Returns an Jsonrpc response for a script dialog. |
| [mympd.gpio_blink](scripting/functions/gpio) | Connects to myGPIOd and blinks a GPIO with given timeout and interval. |
| [mympd.gpio_get](scripting/functions/gpio) | Connects to myGPIOd and returns the active state of a GPIO. |
| [mympd.gpio_set](scripting/functions/gpio) | Connects to myGPIOd and sets the active value of a GPIO. |
| [mympd.gpio_toggle](scripting/functions/gpio) | Connects to myGPIOd and toggles the active value of a GPIO. |
| [mympd.hash_md5](scripting/functions/util) | MD5 hash of string. |
| [mympd.hash_sha1](scripting/functions/util) | SHA1 hash of string. |
| [mympd.hash_sha256](scripting/functions/util) | SHA256 hash of string. |
| [mympd.htmlencode](scripting/functions/util) | Simple HTML encoding. |
| [mympd.http_client](scripting/functions/http_client) | Simple HTTP client. |
| [mympd.http_download](scripting/functions/http_client) | Download a file over http. |
| [mympd.http_jsonrpc_error](scripting/functions/http_replies) | Sends a JSONRPC 2.0 error. |
| [mympd.http_jsonrpc_response](scripting/functions/http_replies) | Sends a JSONRPC 2.0 response. |
| [mympd.http_jsonrpc_warn](scripting/functions/http_replies) | Sends a JSONRPC 2.0 warning. |
| [mympd.http_redirect](scripting/functions/http_replies) | Returns a valid HTTP redirect message. |
| [mympd.http_reply](scripting/functions/http_replies) | Returns a valid HTTP response message. |
| [mympd.http_serve_file](scripting/functions/http_replies) | Serves a file from the filesystem. Only files from the diskcache are allowed. |
| [mympd.init](scripting/functions/mympd_init) | Initializes the Lua table mympd_state. |
| [mympd.log](scripting/functions/util) | Logging to myMPD log. |
| [mympd.notify_client](scripting/functions/util) | Sends a notification to the client. |
| [mympd.notify_partition](scripting/functions/util) | Sends a notification to all clients in a partition. |
| [mympd.os_capture](scripting/functions/system_command) | Executes a system command and capture its output. |
| [mympd.tmp_file](scripting/functions/diskcache) | Generates a random tmp filename for the misc cache. |
| [mympd.update_mtime](scripting/functions/diskcache) | Updates the timestamp of a file. |
| [mympd.urldecode](scripting/functions/util) | Decodes a URL encoded string. |
| [mympd.urlencode](scripting/functions/util) | URL encodes a string. |

- [LuaDoc](luadoc/files/debug/contrib/lualibs/mympd.html)
