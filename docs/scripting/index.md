---
layout: page
permalink: /scripting/
title: Scripting
---

myMPD integrates [Lua](http://www.lua.org) for scripting purposes. There are two types of scripts.

The first type of scripts are executed by triggers, timers or manual though the web ui. This scripts are executed asynchronously and can not block the main thread of myMPD. The script output is printed to STDOUT and the return value is broadcasted to all connected clients in the current partition.

The second type of script are called by http requests (`/script/<partition>/<script>`) and are executed in the context of the webserver. This scripts should return a valid http response including status code, headers and body.

## Global variables

myMPD populates automatically some global variables.

| VARIABLE | DESCRIPTION |
| -------- | ----------- |
| `arguments` | Script arguments |
| `partition` | MPD partition |
| `scriptevent` | Script start event: `extern`, `http`, `timer`, `trigger` or `user` |
| `scriptname` | Script name |

### Arguments

Script arguments are populated in the lua table `arguments`.

```lua
-- return the argument named playlist
return arguments["playlist"]
```

## Custom myMPD lua functions

myMPD provides custom lua functions through the `mympd` lua library.

| FUNCTION | DESCRIPTION |
| -------- | ----------- |
| `mympd.api` | Access to the myMPD API. |
| `mympd.gpio_blink` | Connects to myGPIOd and blinks a GPIO with given timeout and interval. |
| `mympd.gpio_get` | Connects to myGPIOd and returns the active state of a GPIO. |
| `mympd.gpio_set` | Connects to myGPIOd and sets the active value of a GPIO. |
| `mympd.gpio_toggle` | Connects to myGPIOd and toggles the active value of a GPIO. |
| `mympd.hash_sha1` | SHA1 hash of string. |
| `mympd.hash_sha256` | SHA256 hash of string. |
| `mympd.http_client` | Simple HTTP client. |
| `mympd.http_redirect` | Returns a valid HTTP redirect message. |
| `mympd.http_reply` | Returns a valid HTTP response message. |
| `mympd.init` | Initializes the [Lua table mympd_state]({{ site.baseurl }}/scripting/lua-table-mympd_state). |
| `mympd.os_capture` | Executes a system command and capture its output. |
| `mympd.urldecode` | Decodes a URL encoded string. |
| `mympd.urlencode` | URL encodes a string. |
{: .table .table-sm }

### Accessing the myMPD API

Calls the myMPD API, look at [API]({{ site.baseurl }}/references/api/) for detailed API description.

```lua
rc, result = mympd.api("method", params)
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

### Hashing functions

```lua
sha1_hash = mympd.hash_sha1(string)
sha256_hash = mympd.hash_sha256(string)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| string | string | String to hash |
{: .table .table-sm }

### HTTP client

A simple http client.

```lua
rc, code, header, body = mympd.http_client(method, uri, headers, payload)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| method | string | HTTP method, `GET` or `POST` |
| uri | string | full uri to call, e. g. `https://api.listenbrainz.org/1/submit-listens` |
| headers | string | must be terminated by `\r\n` |
| payload | string | body of a post request |
{: .table .table-sm }

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = success, 1 = error |
| code | integer | http response code, e.g. 200 |
| header | string | http headers |
| body | string | http body |
{: .table .table-sm }

### Accessing myMPD and MPD status information

Populates the lua table `mympd_state` with configuration values and current status of myMPD and MPD.

Additionally all user defined variables are populates in this table. They are prefixed with `var_`.

```lua
mympd.init()
```

**Parameters:**

No parameters needed.

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = success, 1 = error |
| result | lua table | jsonrpc result or error as lua table |
{: .table .table-sm }

- [Lua table mympd_state]({{ site.baseurl }}/scripting/lua-table-mympd_state)

### Execute a system command

Executes a system command and captures its output.

```lua
output = mympd.os_capture(command)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| command | string | system command to execute |
{: .table .table-sm }

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| output | string | system command output as lua string |
{: .table .table-sm }

If you want to run commands that changes the effective userid (e.g. with `sudo`) and you run myMPD with the default systemd service unit, you must create the mympd user manually and add an override.

```sh
groupadd -r mympd
useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd

curl -s https://raw.githubusercontent.com/jcorporation/myMPD/v10.0.0/contrib/initscripts/mympd.service.in | sed 's|@CMAKE_INSTALL_FULL_BINDIR@|/usr/bin|' /etc/systemd/system/mympd.service

systemctl daemon-reload
systemctl restart mympd
```

### URL encoding and decoding

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

### HTTP replies

This functions are creating raw http response for usage in scripts called by http requests.

```lua
-- Return a complete http reply
local status = 200
local headers ="Content-type: text/plain\r\n"
local body = "testbody"
return mympd.http_reply(status, headers, body)

-- Return a 302 FOUND response (temporary redirect) to /test
local location = "/test"
return mympd.http_redirect(location)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| status | integer | HTTP status code, e.g. 200 |
| headers | string | HTTP headers to append, terminate each header with `\r\n`. `Status`, `Connection` and `Content-Length` headers are added automatically. |
| body | string | Response body |
{: .table .table-sm }

### GPIO interface

The GPIO interface requires a configured [myGPIOd](https://github.com/jcorporation/myGPIOd).

All functions are connecting to the socket `/run/mygpiod/socket`, issues the command and disconnects.

```lua
-- Set non default socket for myGPIOd connection
mympd.mygpiod_socket = "/run/mygpiod/socket.debug"

-- Blink a GPIO at given timeout and interval
rc = mympd.gpio_blink(gpio, timeout_ms, interval_ms)

-- Get the active state of a GPIO
-- 0 = inactive, 1 = active
state = mympd.gpio_get(gpio)

-- Sets the active state of a GPIO
-- 0 = inactive, 1 = active
rc = mympd.gpio_set(gpio, state)

-- Toggles the active state of a GPIO
rc = mympd.gpio_toggle(gpio)
```

## Lua manual

- [Lua manual](https://www.lua.org/manual/5.4/)

## Examples

Complete scripts can be found in the [repository](https://github.com/jcorporation/myMPD/tree/master/docs/scripting/scripts).

### Simple

```lua
-- load a playlist
mympd.api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {plist = "NonPop"})
-- start playing
mympd.api("MYMPD_API_PLAYER_PLAY")
```

### With arguments

Script should be called with an argument named playlist.

```lua
-- load a playlist
mympd.api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {plist = arguments["playlist"]})
-- start playing
mympd.api("MYMPD_API_PLAYER_PLAY")
-- broadcast message to all connected myMPD clients
return("Loaded playlist: " .. arguments["playlist"])
```

### Error handling

```lua
-- get current playing song
rc, result = mympd.api("MYMPD_API_PLAYER_CURRENT_SONG", {})
if rc == 0
  return "Current song title: " .. result["Title"]
else
  return "Error message: " .. result["message"]
end
```

## Lua standard libraries

myMPD loads in the default config all lua standard libraries and the myMPD custom libraries. The configuration file lualibs controls which libraries myMPD opens before script execution.

**Valid values are:**

- Use `all` to load all standard lua libraries and the myMPD custom libraries
- Available standard lua libraries: base, coroutine, debug, io, math, os, package, string, table, utf8
- Available myMPD custom libraries:
  - [mympd](https://github.com/jcorporation/myMPD/blob/master/contrib/lualibs/mympd.lua)
  - [json](https://github.com/rxi/json.lua)

## Script file format

Scripts are saved in the directory `/var/lib/mympd/scripts` with the extension `.lua`. The metadata (order, arguments) are saved in the first line in a lua comment as json object.

```lua
-- {"order":1,"arguments":["testarg1", "testarg2"]}
return("Arguments are: " .. arguments["testarg1"] .. arguments["testarg2"])
```

| OPTION | DESCRIPTION |
| ------ | ----------- |
| order | Sort order of the script, 0 disables listing in main menu |
| arguments | Name of the keys for the script arguments, the gui asks for this arguments. Arguments are populated in a lua table called arguments. |
{: .table .table-sm }
