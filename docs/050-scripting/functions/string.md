---
title: String Functions
---

Some useful string handling functions.

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
