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
| [mympd.covercache_write]({{site.baseurl}}/scripting/functions/covercache) | Writes a covercache file. |
| [mympd.gpio_blink]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and blinks a GPIO with given timeout and interval. |
| [mympd.gpio_get]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and returns the active state of a GPIO. |
| [mympd.gpio_set]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and sets the active value of a GPIO. |
| [mympd.gpio_toggle]({{site.baseurl}}/scripting/functions/gpio) | Connects to myGPIOd and toggles the active value of a GPIO. |
| [mympd.hash_sha1]({{site.baseurl}}/scripting/functions/util) | SHA1 hash of string. |
| [mympd.hash_sha256]({{site.baseurl}}/scripting/functions/util) | SHA256 hash of string. |
| [mympd.http_client]({{site.baseurl}}/scripting/functions/http_client) | Simple HTTP client. |
| [mympd.http_download]({{site.baseurl}}/scripting/functions/http_client) | Download a file over http. |
| [mympd.http_redirect]({{site.baseurl}}/scripting/functions/http_replies) | Returns a valid HTTP redirect message. |
| [mympd.http_reply]({{site.baseurl}}/scripting/functions/http_replies) | Returns a valid HTTP response message. |
| [mympd.init]({{site.baseurl}}/scripting/functions/mympd_init) | Initializes the Lua table mympd_state. |
| [mympd.log]({{site.baseurl}}/scripting/functions/util) | Logging to myMPD log. |
| [mympd.notify_client]({{site.baseurl}}/scripting/functions/util) | Sends a notification to the client. |
| [mympd.notify_partition]({{site.baseurl}}/scripting/functions/util) | Sends a notification to all clients in a partition. |
| [mympd.os_capture]({{site.baseurl}}/scripting/functions/system_command) | Executes a system command and capture its output. |
| [mympd.urldecode]({{site.baseurl}}/scripting/functions/util) | Decodes a URL encoded string. |
| [mympd.urlencode]({{site.baseurl}}/scripting/functions/util) | URL encodes a string. |
{: .table .table-sm }
