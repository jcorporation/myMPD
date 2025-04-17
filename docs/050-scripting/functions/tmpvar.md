---
title: Temporary Variables
---

Temporary variables are kept in memory only and have an expiration time. They are useful to save some state between script executions.

## Expiration

- Positive number: Variable expires in now + lifetime seconds
- `0`: Variable expires by next `mympd.tmpvar_get` call
- `-1`: Variable does not expire

```lua
-- Set a tmp variable
local rc, result = mympd.tmpvar_set(key, value, lifetime)

-- Get a tmp variable
local value, expiration = mympd.tmpvar_get(key)

-- List all tmp variables
local vars = mympd.tmpvar_list()
for _, v in pairs(vars) do
  mympd.log(6, v.key .. ":" .. v.value .. ":" .. v.expires)
end

-- Delete a tmp variable
local rc, result = mympd.tmpvar_delete(key)
```
