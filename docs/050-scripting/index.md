---
title: Scripting
---

myMPD integrates [Lua](http://www.lua.org) for scripting purposes. Scripts are executed asynchronously and can not block the main thread of myMPD. There are two types of scripts.

The first type of scripts are executed by triggers, timers or manual through the web ui. The script output is printed to STDOUT and the return value is broadcasted to all connected clients in the current partition.

- [Triggers](../060-references/trigger.md)

The second type of script are called by http requests (`/script/<partition>/<script>`), special triggers and home screen widgets. This scripts should return a valid http response including status code, headers and body.

- [Widgets](widgets.md)

## Global variables

myMPD populates automatically some global variables.

myMPD uses function and variable names prefixed with `mympd`. You should avoid this in your naming convention.

### myMPD environment

myMPD environment variables are populated in the lua table `mympd_env`.

| KEY | TYPE | DESCRIPTION |
| --- | ---- | ----------- |
| `cachedir_cover` | string | myMPD cover cache directory |
| `cachedir_thumbs` | string | myMPD thumbs cache directory |
| `cachedir_lyrics` | string | myMPD lyrics cache directory |
| `cachedir_misc` | string | myMPD misc cache directory |
| `partition` | string | MPD partition |
| `requestid` | number | Jsonrpc request id |
| `scriptevent` | string | Script start event: `extern`, `http`, `timer`, `trigger` or `user` |
| `scriptname` | string | Script name |
| `var` | table | Subtable with user defined variables |
| `workdir` | string | myMPD working directory |

Additionally all user defined variables are populates in this table. They are prefixed with `var_`.

### Arguments

Script arguments are populated in the lua table `mympd_arguments`. All arguments are of type string.

```lua
-- return the argument named playlist
return mympd_arguments["playlist"]
```

Arguments can be defined as simple names (text input field) or with typical html form element types.

| DEFINITION | DESCRIPTION |
| ---------- | ----------- |
| `<argument>\|text` | Text input field. |
| `<argument>\|password` | Password input field. |
| `<argument>\|hidden` | Hidden input field. |
| `<argument>\|select;opt1;opt2;opt3` | Selectbox, options are separated by semicolon. |
| `<argument>\|checkbox`| Simple checkbox, returns the string `true` if checked. |
| `<argument>\|radio;r1;r2;r3` | Radioboxes, returns the selected option. Options are separated by semicolon. |
| `<argument>\|list;r1;r2;r3` | List of values, returns the selected values separated by `;;`. Values are separated by semicolon. |

### myMPD state

The mympd_state global lua table is NOT populates automatically. You must call `mympd.init()` to populate it.

- [mympd.init](functions/mympd_init.md)

## Custom myMPD lua functions

myMPD provides custom lua functions through the `mympd` and `json` lua library.

- [List of all functions](functions/index.md)

## Lua manual

- [Lua manual](https://www.lua.org/manual/5.4/)

## Examples

Here can you find some simple examples.

Complete scripts that can be imported are in the [mympd-scripts repository](https://github.com/jcorporation/mympd-scripts).

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
mympd.api("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {plist = mympd_arguments.playlist})
-- start playing
mympd.api("MYMPD_API_PLAYER_PLAY")
-- broadcast message to all connected myMPD clients
return("Loaded playlist: " .. mympd_arguments.playlist)
```

### Error handling

```lua
-- get current playing song
rc, result = mympd.api("MYMPD_API_PLAYER_CURRENT_SONG", {})
if rc == 0
  return "Current song title: " .. result.Title
else
  return "Error message: " .. result.message
end
```

## Script file format

Scripts are saved in the directory `/var/lib/mympd/scripts` with the extension `.lua`. The metadata is saved in the first line in a lua comment as json object.

```lua
-- {"name": "scriptname", "file": "category/scriptname.lua", "version": 1, "desc": "short description", "order":1,"arguments":["testarg1", "testarg2"]}
return("Arguments are: " .. mympd_arguments.testarg1 .. mympd_arguments.testarg2)
```

### Metadata

| KEY | DESCRIPTION |
| --- | ----------- |
| name | Friendly name of the script (for importing scripts). |
| file | Script filename in the mympd-scripts repository. |
| version | Version number of the script. |
| desc | A short description. |
| order | Sort order of the script, 0 disables listing in main menu. |
| arguments | Name of the keys for the script arguments, the gui asks for this arguments. Arguments are populated in a lua table called `mympd_arguments`. |
