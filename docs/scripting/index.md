---
layout: page
permalink: /scripting/
title: Scripting
---

myMPD integrates [Lua](http://www.lua.org) for scripting purposes. Script execution can be triggered in the main menu, with home icons, timers or triggers. Scripts are executed asynchronously, therefore scripts can not block the main threads of myMPD. The script output is printed to STDOUT and the return value is broadcasted to all connected clients in the current partition.

## Arguments

Script arguments are populated in the lua table `arguments`. myMPD populates automatically the global `partition` variable.

## Custom myMPD lua functions

myMPD provides custom lua functions through the `mympd` lua library.

| FUNCTION | DESCRIPTION |
| -------- | ----------- |
| `mympd.api` | Access to the myMPD API |
| `mympd.http_client` | Simple HTTP client |
| `mympd.init` | Initializes the [Lua table mympd_state]({{ site.baseurl }}/scripting/lua-table-mympd_state) |
| `mympd.os_capture` | Executes a system command and capture its output. |
{: .table .table-sm }

### Accessing the myMPD API

Calls the myMPD API, look at [API]({{ site.baseurl }}/references/api/) for detailed API description.

```
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

### HTTP client

A simple http client.

```
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

```
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

```
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

```
groupadd -r mympd
useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd

curl -s https://raw.githubusercontent.com/jcorporation/myMPD/v10.0.0/contrib/initscripts/mympd.service.in | sed 's|@CMAKE_INSTALL_FULL_BINDIR@|/usr/bin|' /etc/systemd/system/mympd.service

systemctl daemon-reload
systemctl restart mympd
```

## Lua manual

- [Lua manual](https://www.lua.org/manual/5.4/)

## Examples

Complete scripts can be found in the [repository](https://github.com/jcorporation/myMPD/tree/master/docs/scripting/scripts).

### Simple

```
-- load a playlist
mympd.api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {plist = "NonPop"})
-- start playing
mympd.api("MYMPD_API_PLAYER_PLAY")
```

### With arguments

Script should be called with an argument named playlist.

```
-- load a playlist
mympd.api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {plist = arguments["playlist"]})
-- start playing
mympd.api("MYMPD_API_PLAYER_PLAY")
-- broadcast message to all connected myMPD clients
return("Loaded playlist: " .. arguments["playlist"])
```

### Error handling

```
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

```
-- {"order":1,"arguments":["testarg1", "testarg2"]}
return("Arguments are: " .. arguments["testarg1"] .. arguments["testarg2"])
```

| OPTION | DESCRIPTION |
| ------ | ----------- |
| order | Sort order of the script, 0 disables listing in main menu |
| arguments | Name of the keys for the script arguments, the gui asks for this arguments. Arguments are populated in a lua table called arguments. |
{: .table .table-sm }
