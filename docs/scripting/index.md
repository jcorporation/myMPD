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

myMPD uses function and variable names prefixed with `mympd`. You should avoid this in your naming convention.

### myMPD environment

myMPD environment variables are populated in the lua table `mympd_env`.

| KEY | TYPE | DESCRIPTION |
| -------- | ---- | ----------- |
| `cachedir` | string | myMPD cache directory |
| `cachedir_cover` | string | myMPD covercache directory |
| `partition` | string | MPD partition |
| `requestid` | number | Jsonrpc request id |
| `scriptevent` | string | Script start event: `extern`, `http`, `timer`, `trigger` or `user` |
| `scriptname` | string | Script name |
| `workdir` | string | myMPD working directory |

### Arguments

Script arguments are populated in the lua table `mympd_arguments`.

```lua
-- return the argument named playlist
return mympd_arguments["playlist"]
```

### myMPD state

The mympd_state global lua table is NOT populates automatically. You must call `mympd.init()` to populate it.

- [mympd.init]({{site.baseurl}}/scripting/functions/mympd_init)

## Custom myMPD lua functions

myMPD provides custom lua functions through the `mympd` and `json` lua library.

- [List of all functions]({{site.baseurl}}/scripting/functions/)

## Lua manual

- [Lua manual](https://www.lua.org/manual/5.4/)

## Examples

Complete scripts can be found in the [repository](https://github.com/jcorporation/myMPD/tree/master/docs/scripting/scripts).

- [Usage examples]({{site.baseurl}}/scripting/usage/)

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
mympd.api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {plist = mympd_arguments["playlist"]})
-- start playing
mympd.api("MYMPD_API_PLAYER_PLAY")
-- broadcast message to all connected myMPD clients
return("Loaded playlist: " .. mympd_arguments["playlist"])
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
  - [mympd](https://github.com/jcorporation/myMPD/blob/master/contrib/lualibs/mympd)
  - [json](https://github.com/rxi/json.lua)

## Script file format

Scripts are saved in the directory `/var/lib/mympd/scripts` with the extension `.lua`. The metadata (order, arguments) are saved in the first line in a lua comment as json object.

```lua
-- {"order":1,"arguments":["testarg1", "testarg2"]}
return("Arguments are: " .. mympd_arguments["testarg1"] .. mympd_arguments["testarg2"])
```

| OPTION | DESCRIPTION |
| ------ | ----------- |
| order | Sort order of the script, 0 disables listing in main menu |
| arguments | Name of the keys for the script arguments, the gui asks for this arguments. Arguments are populated in a lua table called `mympd_arguments`. |
{: .table .table-sm }
