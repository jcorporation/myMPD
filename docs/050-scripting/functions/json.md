---
layout: page
permalink: /scripting/functions/json
title: Json
---

Encoding and decoding of Json to/from Lua tables.

```lua
-- Encode a Lua table as Json
local payload = json.encode({
    recording_mbid = mbid,
    blurb_content = arguments["blurb_content"],
    pinned_until = arguments["pinned_until"]
});

-- Decode Json as Lua table
local string = "{\"str\":\"value\", \"number\": 1}"
local decoded = json.decode(string)
print(decoded.str)
print(decoded.number)
```
