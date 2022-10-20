---
layout: page
permalink: /scripting/
title: Scripting
---

myMPD integrates [Lua](http://www.lua.org) for scripting purposes. Script execution can be triggered in the main menu, with timers or triggers. Scripts are executed asynchronously, therefore scripts can not block the main threads of myMPD. The script output is printed to STDOUT and the return value is broadcasted to all connected clients.

## Arguments

Script arguments are populated in the lua table `arguments`. myMPD populates automatically the global `partition` variable.

## Accessing myMPD and MPD status informations

Accessing myMPD requires the mympd lua library to be loaded.

The lua command `mympd.init()` populates the lua table `mympd_state` with configuration values and up-to-date status information's of myMPD and MPD. `mympd.init()` is only a shorthand command for `mympd_api("INTERNAL_API_SCRIPT_INIT")`

- [Lua table mympd_state]({{ site.baseurl }}/scripting/lua-table-mympd_state)

## myMPD API functions

| FUNCTION | DESCRIPTION |
| -------- | ----------- |
| `mympd.init()` | Initializes the [Lua table mympd_state]({{ site.baseurl }}/scripting/lua-table-mympd_state) |
| `mympd.os_capture(<command>)` | Executes a system command and capture its output. |
{: .table .table-sm }

### mympd_api

The lua command `mympd_api("method", "key1", "value1", ...)` executes myMPD API functions, look at [API]({{ site.baseurl }}/references/api/) for detailed API description. The first argument must be the API method, followed by key / value pairs.

The first return value is an error code (0 = OK, 1 = ERROR). If the return value of the API function is only a message or a error message the second return value is a simple string else the original json string is returned.

### mympd_api_raw

The lua command `mympd_api_raw("method", "params")` executes myMPD API functions, look at [API]({{ site.baseurl }}/references/api/) for detailed API description. The first argument must be the API method, followed by a json string describing the params, e.g. `json.encode({key1 = "value1", key2 = {"val2", "val3"}})`

The first return value is an error code (0 = OK, 1 = ERROR). The second return value is the unparsed json string.

### mympd_api_http_client

This lua command executes an http request.

- method: HTTP method, `GET` or `POST`
- uri: full uri to call, e. g. `https://api.listenbrainz.org/1/submit-listens`
- headers: must be terminated by `\r\n`.
- payload: body of a post request

```
rc, response, header, body = mympd_api_http_client(method, uri, headers, payload)
```

## Lua manual

- [Lua manual](https://www.lua.org/manual/5.4/)

## Examples

Further examples can be found in the [repository](https://github.com/jcorporation/myMPD/tree/master/docs/scripting/scripts).

### Simple

```
-- load a playlist
mympd_api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", "plist", "NonPop")
-- start playing
mympd_api("MYMPD_API_PLAYER_PLAY")
```

### With arguments

Script should be called with an argument named playlist.
```
-- load a playlist
mympd_api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", "plist", arguments["playlist"])
-- start playing
mympd_api("MYMPD_API_PLAYER_PLAY")
-- broadcast message to all connected myMPD clients
return("Loaded playlist: " .. arguments["playlist"])
```

### JSON parsing

```
rc, raw_result = mympd_api("MYMPD_API_PLAYER_CURRENT_SONG")
if rc == 0 then
  current_song = json.decode(raw_result)
  return current_song["result"]["Artist"]
else
  return "No current song"
end
```

### Executing a system command

```
output = mympd.os_capture("echo test")
return output
```

## mympd-script

`mympd-script` is a small commandline tool to submit scripts to myMPD. It reads the script from STDIN and submits it to myMPD for execution. `Key=Value` parameters can be used to fill the arguments table in the Lua script.

For security reasons this function has a default acl of `-0.0.0.0/0,+127.0.0.0/8` in the default configuration. There is a IP ACL option `scriptacl` in the config folder to   override the default acl.

**Script from STDIN:**
```
mympd-script https://localhost default - key1=value1 <<< 'print arguments["key1"]'
```

**Call available script (test.lua):**

mympd-script can also call existing scripts. This API call is not controlled by the remote scripts configuration options.

```
mympd-script https://localhost default test key1=value1 
```

## LUA standard libraries

myMPD loads in the default config all lua standard libraries. The config option lualibs controls which libraries myMPD opens before script execution.

- Loaded by default: all
- Available (standard lua libraries): base, coroutine, debug, io, math, os, package, string, table, utf8  
- Available (myMPD custom libraries):
  - mympd
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
