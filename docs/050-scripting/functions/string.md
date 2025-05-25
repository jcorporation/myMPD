---
title: String Functions
---

Some useful string handling functions.

## Check for empty string

Checks for empty string or nil.

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| str | string | String to check. |

```lua
if mympd.isnilorempty(str) then

end
```

## Splitting strings

Split string by newline characters and trims the lines.

```lua
local lines = mympd.splitlines(str)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| str | string | Multiline string to split. |

## Triming strings

Removes beginning and ending whitespaces from a string.

```lua
local trimed = mympd.trim(str)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| str | string | String to trim. |

## Match table values against a comma separated list

Checks a Lua table of tags against a comma separated list.

local list_str = "Speech, Podcast, Audio Book"
local tbl = { "Speech", "Soundtrack" }

```lua
if mympd.tblvalue_in_list(list_str, tbl) == true then
    -- Handle match
end
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| list_str | string | Comma separated list values |
| tbl | table | Lua table of values to check against the list |
