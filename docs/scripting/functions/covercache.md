---
layout: page
permalink: /scripting/functions/covercache
title: Covercache
---

Helper function to write a file to the covercache. The source file must be on the same filesystem as the covercache directory (default: `/var/cache/mympd/covercache`).

```lua
local rc = mympd.covercache_write(src, uri)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| uri | string | Uri to write the covercache for. |
{: .table .table-sm }

**Returns:**

0 on success.
